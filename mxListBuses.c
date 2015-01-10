/*=================================================================
 * mxListBuses.c 
 *
 *=================================================================*/

#include "mxDataIO.h"

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
#include "_SetUpConnection_global_declaration.h"
#include "_SetUpConnection_client_declaration.h"
    
    void *Buffer;
    DS_ListBusResponse *listResp;
    
	char **Buses_ptr, *Times_ptr[2]; 
    char *Name_string, StatusText[2048];
    int *FrameCountValue_ptr;
    int64 *TimeValue_ptr;
    mxArray *TimeStruct_ptr, *FieldsStruct_ptr, *FrameCountsStruct_ptr, *Data_ptr;
    mwSize dims[2];
    int i, j, elementCount, unhandledType;
    
    
    //////////////////////////////////////////////////////////////////////////
	// 1. Check for proper number of input and output arguments
	//////////////////////////////////////////////////////////////////////////
    
    if (!(nrhs == 1))
        mexErrMsgTxt("Input argument combination allowed is [struct].\n");
    
    if (!(!(mxIsEmpty(prhs[0])) && mxIsStruct(prhs[0])))
        mexErrMsgTxt("First input argument is not a struct.\n");
    
    if (nlhs > 0 && !(nlhs == 2 || nlhs == 3))
        mexErrMsgTxt("Two or three output arguments are required.\n");
    
    
    //////////////////////////////////////////////////////////////////////////
	// 2. Connect to the MAPPS shared memory buffer
	//////////////////////////////////////////////////////////////////////////    
    
#include "_SetUpConnection_global_code.h"
#include "_SetUpConnection_client_code.h"
    
    Buffer = ConnectToClient(&h2, client);
	
	if(Buffer == NULL)
	{
		DisconnectFromGlobal(h1, global);
		mexErrMsgIdAndTxt("MEX:ListBuses:UnableToConnectClient", "Unable to connect to shared memory buffer (client state)! \n");
	}
		
		
	//////////////////////////////////////////////////////////////////////////
	// 3. Prepare the buffer for use
	//////////////////////////////////////////////////////////////////////////
	
	if(Prepare(global, client, EDS_ListBuses) == FALSE)
	{
		DisconnectFromClient(h2, client);
        DisconnectFromGlobal(h1, global);
		mexErrMsgIdAndTxt("MEX:ListBuses:ClientBlocked", "Server is still blocking the client channel! \n");
	}


	//////////////////////////////////////////////////////////////////////////
	// 4. Execute
	//////////////////////////////////////////////////////////////////////////
	
    if(Execute(global, client, 0) == FALSE)
    {
        DisconnectFromClient(h2, client);
        DisconnectFromGlobal(h1, global);
        mexErrMsgIdAndTxt("MEX:ListBuses:ServerTimedOut", "SERVER TIMED OUT! \n");
    }


	//////////////////////////////////////////////////////////////////////////
	// 5. Echo the result
	//////////////////////////////////////////////////////////////////////////

	if(strncmp(client->Result, "success", 7) == 0)
	{
        listResp = (DS_ListBusResponse *)Buffer;
		if (nlhs == 0) {
            mexPrintf("Global time is %1.3f-%1.3f seconds \n", 
                (float)listResp->GlobalStartTimeMs*0.001f, 
                (float)listResp->GlobalEndTimeMs*0.001f);

            printf("Current time is %1.3f seconds \n", global->CurrentTime);

            elementCount = 0;
            for (i = 0; i < listResp->BusCount; i++) {
                elementCount += listResp->Buses[i].ElementCount;
            }

            mexPrintf("There are %d bus(es), %d element(s) \n", listResp->BusCount, elementCount);

            mexPrintf("------------------------------------------------- \n");

            for(i=0; i<listResp->BusCount; i++)
            {
                mexPrintf("Bus %d: %s [%1.3f-%1.3f] has %d elements and %d frames\n",
                    i+1, 
                    listResp->Buses[i].BusName,
                    (float)listResp->Buses[i].StartTimeMs*0.001f, 
                    (float)listResp->Buses[i].EndTimeMs*0.001f,
                    listResp->Buses[i].ElementCount,
                    listResp->Buses[i].FrameCount);
            }
            
            mexPrintf("------------------------------------------------- \n");
            mexPrintf("Elements are: \n");
            elementCount = 1;
            for (i = 0; i < listResp->BusCount; i++) {
                for (j = 0; j < listResp->Buses[i].ElementCount; j++) {
                    mexPrintf("  %d: %s.%s \n", elementCount, 
                                                listResp->Buses[i].BusName, 
                                                listResp->Buses[i].Elements[j].ElementName);
                    elementCount++;
                }
            }
        }
	}
	else
		mexPrintf("Details (mxListBuses): %s\n", client->Details);
    
    
    //////////////////////////////////////////////////////////////////////////
	// 6. Populate Matlab output
	//////////////////////////////////////////////////////////////////////////
    
    if (nlhs == 2 || nlhs == 3)
    {
        if (strncmp(client->Result, "success", 7) == 0)
        {
            Buses_ptr = (char **)mxCalloc((mwSize)listResp->BusCount, (mwSize)sizeof(char *));
            for (i = 0; i < listResp->BusCount; i++) {
                Buses_ptr[i] = listResp->Buses[i].BusName;
            }
            dims[0] = 1;
            dims[1] = 1;
            FieldsStruct_ptr = mxCreateStructArray(2, dims, (int)listResp->BusCount, (const char **)Buses_ptr);

            TimeStruct_ptr = mxDuplicateArray(FieldsStruct_ptr);
            Name_string = "Global_MAPPS_Time";
            mxAddField(TimeStruct_ptr, Name_string);
            
            if (nlhs == 3)
                FrameCountsStruct_ptr = mxDuplicateArray(FieldsStruct_ptr);

            Times_ptr[0] = "StartTime";
            Times_ptr[1] = "EndTime";
            for (i = 0; i < listResp->BusCount + 1; i++) {
                if (i == listResp->BusCount) {
                    dims[0] = 1;
                    dims[1] = 1;
                    mxSetField(TimeStruct_ptr, 0, Name_string, mxCreateStructArray(2, dims, 2, (const char **)Times_ptr));
                }
                else {
                    dims[0] = 1;
                    dims[1] = 1;
                    mxSetField(TimeStruct_ptr, 0, (const char *)Buses_ptr[i], mxCreateStructArray(2, dims, 2, (const char **)Times_ptr));
                }

                for (j = 0; j < 2; j++) {
                    TimeValue_ptr = (int64 *)mxCalloc(1, (mwSize)sizeof(int64));
                    if (i == listResp->BusCount) {
                        if (j == 0) {
                            *TimeValue_ptr = (int64)listResp->GlobalStartTimeMs;
                        }
                        else {
                            *TimeValue_ptr = (int64)listResp->GlobalEndTimeMs;
                        }
                    }
                    else {
                        if (j == 0) {
                            *TimeValue_ptr = (int64)listResp->Buses[i].StartTimeMs;
                        }
                        else {
                            *TimeValue_ptr = (int64)listResp->Buses[i].EndTimeMs;
                        }
                    }
                    dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
                    dims[1] = 1;
                    Data_ptr = mxCreateNumericArray(2, dims, mxINT64_CLASS, mxREAL);
                    mxSetM(Data_ptr, 1);
                    mxSetData(Data_ptr, TimeValue_ptr); 

                    if (i == listResp->BusCount) {
                        mxSetField(mxGetField(TimeStruct_ptr, 0, Name_string), 0, Times_ptr[j], Data_ptr);
                    }
                    else {
                        mxSetField(mxGetField(TimeStruct_ptr, 0, (const char *)Buses_ptr[i]), 0, Times_ptr[j], Data_ptr);
                    }
                }
            }

            for (i = 0; i < listResp->BusCount; i++) {
                for (j = 0; j < listResp->Buses[i].ElementCount; j++) {
                    if (j == 0) {
                        Name_string = listResp->Buses[i].Elements[j].ElementName;
                        dims[0] = 1;
                        dims[1] = 1;
                        mxSetField(FieldsStruct_ptr, 0, Buses_ptr[i], mxCreateStructArray(2, dims, 1, (const char **)&Name_string));
                    }
                    else {
                        mxAddField(mxGetField(FieldsStruct_ptr, 0, (const char *)Buses_ptr[i]), listResp->Buses[i].Elements[j].ElementName);
                    }
                    unhandledType = 0;
                    switch(listResp->Buses[i].Elements[j].Type){
                        case MappsTypeInt16:
                            dims[0] = 0;
                            dims[1] = 1;
                            Data_ptr = mxCreateNumericArray(2, dims, mxINT16_CLASS, mxREAL);
                            break;
                        case MappsTypeInt:
                            dims[0] = 0;
                            dims[1] = 1;
                            Data_ptr = mxCreateNumericArray(2, dims, mxINT32_CLASS, mxREAL);
                            break;
                        case MappsTypeInt64:
                            dims[0] = 0;
                            dims[1] = 1;
                            Data_ptr = mxCreateNumericArray(2, dims, mxINT64_CLASS, mxREAL);
                            break;
                        case MappsTypeFloat:
                            dims[0] = 0;
                            dims[1] = 1;
                            Data_ptr = mxCreateNumericArray(2, dims, mxSINGLE_CLASS, mxREAL);
                            break;
                        case MappsTypeDouble:
                            dims[0] = 0;
                            dims[1] = 1;
                            Data_ptr = mxCreateNumericArray(2, dims, mxDOUBLE_CLASS, mxREAL);
                            break;
                        case MappsTypeString:
                            dims[0] = 1;
                            dims[1] = listResp->Buses[i].Elements[j].SizeInBytes - 1;
                            Data_ptr = mxCreateCharArray(2, dims);
                            break;
                        case MappsTypeBlob:
                            mexPrintf("Element '", Name_string, "' is of type 'Blob'.");
                            unhandledType = 1;
                            break;
                        case MappsTypeVariable:
                            mexPrintf("Element '", Name_string, "' is of type 'Variable'.");
                            unhandledType = 2;
                            break;
                        default:
                            unhandledType = 3;
                            break;
                    }
                    if (unhandledType > 0) {
                        sprintf(StatusText, "Unhandled type of class [%d] for %s.", unhandledType, Name_string);
                        if (strlen(StatusText) + 1 > sizeof(client->Status)) {
                            strncpy(client->Status, StatusText, sizeof(client->Status) - 1);
                            client->Status[sizeof(client->Status) - 1] = '\0';
                        }
                        else
                            strncpy(client->Status, StatusText, strlen(StatusText) + 1);                            
                        DisconnectFromClient(h2, client);
                        DisconnectFromGlobal(h1, global);                    
                        mexErrMsgTxt(StatusText);
                    }
                    mxSetField(mxGetField(FieldsStruct_ptr, 0, (const char *)Buses_ptr[i]), 0, listResp->Buses[i].Elements[j].ElementName, Data_ptr);                   
                }
            }
            
            if (nlhs == 3) {
                for (i = 0; i < listResp->BusCount; i++) {
                    FrameCountValue_ptr = (int *)mxCalloc(1, (mwSize)sizeof(int));
                    *FrameCountValue_ptr = (int)(listResp->Buses[i].FrameCount);
                                                            
                    dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, FrameCountValue_ptr)
                    dims[1] = 1;
                    Data_ptr = mxCreateNumericArray(2, dims, mxINT32_CLASS, mxREAL);
                    mxSetM(Data_ptr, 1);
                    mxSetData(Data_ptr, FrameCountValue_ptr);
                    
                    mxSetField(FrameCountsStruct_ptr, 0, (const char *)Buses_ptr[i], Data_ptr);
                }
            }

            plhs[0] = TimeStruct_ptr;
            plhs[1] = FieldsStruct_ptr;
            if (nlhs == 3)
                plhs[2] = FrameCountsStruct_ptr;

            mxFree(Buses_ptr);
        }
        else 
        {
            dims[0] = 0;
            dims[1] = 1;
            plhs[0] = mxCreateNumericArray(2, dims, mxDOUBLE_CLASS, mxREAL);
            plhs[1] = mxCreateNumericArray(2, dims, mxDOUBLE_CLASS, mxREAL);
            if (nlhs == 3) 
                plhs[2] =  mxCreateNumericArray(2, dims, mxDOUBLE_CLASS, mxREAL);
        }
    }
    
	
	//////////////////////////////////////////////////////////////////////////
	// 7. Disconnect from the MAPPS shared memory buffer
	//////////////////////////////////////////////////////////////////////////
	
	DisconnectFromClient(h2, client);
    DisconnectFromGlobal(h1, global);
}
        

        /*
        flag_StructCreated = (int *)mxCalloc((mwSize)listResp->BusListResponse.BusCount, (mwSize)sizeof(int));
        for (i = 0; i < listResp->BusListResponse.BusCount; i++) {
            flag_StructCreated[i] = 0;
        }
        Name_string = strtok((char *)&(listResp->BusListResponse.Elements), "|");
        for (i = 0; i < listResp->BusListResponse.ElementCount; i++) {
            for (j = 0; j < listResp->BusListResponse.BusCount; j++) {
                length = (int)strlen((const char *)Buses_ptr[j]);
                if (Name_string != NULL && strncmp((const char *)Name_string, (const char *)Buses_ptr[j], length) == 0) {
                    Field_name = &(Name_string[length + 1]);
                    if (flag_StructCreated[j] == 0) {
                        dims[0] = 1;
                        dims[1] = 1;
                        mxSetField(FieldsStruct_ptr, 0, Buses_ptr[j], mxCreateStructArray(2, dims, 1, (const char **)&Field_name));
                        flag_StructCreated[j] = 1;
                    }
                    else {
                        mxAddField(mxGetField(FieldsStruct_ptr, 0, (const char *)Buses_ptr[j]), Field_name);
                    }
                    Name_string = strtok(NULL, "|");
                    break;
                }
            }
        } 
        
        mxFree(flag_StructCreated);
        */
        