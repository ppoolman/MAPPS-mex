/*=================================================================
 * mxGetBuses.c 
 *
 *=================================================================*/

#include "mxDataIO.h"

#define RETRIEVE_SINGLE_FIELD_AT_A_TIME  1

mxClassID Interpret_MAPPS_DataType(const char *DataType)
{
    if (strcmp(DataType, "uchar") == 0)			return mxUINT8_CLASS;
    else if (strcmp(DataType, "int16") == 0)	return mxINT16_CLASS;
    else if (strcmp(DataType, "uint16") == 0) 	return mxUINT16_CLASS;
    else if (strcmp(DataType, "int") == 0) 		return mxINT32_CLASS;
    else if (strcmp(DataType, "uint") == 0) 	return mxUINT32_CLASS;
    else if (strcmp(DataType, "int64") == 0) 	return mxINT64_CLASS;
    else if (strcmp(DataType, "uint64") == 0) 	return mxUINT64_CLASS;
    else if (strcmp(DataType, "float") == 0) 	return mxSINGLE_CLASS;
    else if (strcmp(DataType, "double") == 0) 	return mxDOUBLE_CLASS;
    else if (strcmp(DataType, "string") == 0) 	return mxCHAR_CLASS;  
    else 										return mxUNKNOWN_CLASS;
}

void Retrieve_MAPPS_Data_Single_Field(
	const char *Busname, 
	const char *Fieldname, 
	const int64 fromTime, 
	const int64 toTime,
	void **Data, 
	char **DataType,
    int *BytesPerEntry,
    int **EntriesPerTimeslice,   // for buses with variable-length blobs per timeslice
	void **Timestamps, 
	char **Timestamp_DataType, 
	uint64 *nTimeslices,
	void *global, void *client, void *h1, void *h2, void *Buffer)
{
    /* 1) Search for field "Fieldname" in the MAPPS buffer "Busname"
     * 2) If available, copy elements between "fromTime" and "toTime" into vector, and pass back in "Data"
     * 3) Report data type (single, double, uint64, char, etc.) in "DataType"
     * 4) If Timestamps != NULL (i.e., associated timestamps requested), 
     *    copy MAPPS timestamps between "fromTime" and "toTime" into vector, and pass back in "Timestamps"
     * 5) If Timestamps != NULL (i.e., associated timestamps requested), 
     *    report timestamp data type (single, double, uint64, char, etc.) in "Timestamp_DataType"
     * 6) Report number of timeslices in "nTimeslices" */
    
    uint64 j;
    size_t bufferSize;
    char **elementString;
    DS_RetrieveDataRequest *req; 
	DS_RetrieveDataResponse *resp;
    char* payload;

	*nTimeslices = 0;
    
    //printf("\nBusname = %s\n", Busname);
    //printf("Fieldname = %s\n", Fieldname);
    //printf("fromTime = %lld\n", fromTime);
    //printf("toTime = %lld\n", toTime);


    //////////////////////////////////////////////////////////////////////////
	// (1) Prepare the buffer for use
	//////////////////////////////////////////////////////////////////////////
	
	if(Prepare((EDS_GlobalState *)global, (EDS_Client *)client, EDS_RetrieveBuses) == FALSE)
	{
		DisconnectFromClient(h2, (EDS_Client *)client);
        DisconnectFromGlobal(h1, (EDS_GlobalState *)global);
		mexErrMsgIdAndTxt("MEX:GetBuses:ClientBlocked", "Server is still blocking the client channel! \n");
	}
    

	//////////////////////////////////////////////////////////////////////////
	// (2) Populate
	//////////////////////////////////////////////////////////////////////////

	req = (DS_RetrieveDataRequest *)Buffer;
    bufferSize = 0;
    
	sprintf(req->BusName, Busname);
	if (Fieldname == NULL)
        sprintf(req->ElementNames, "");
    else
        sprintf(req->ElementNames, Fieldname);
	req->StartTimeMilli = fromTime;
	req->EndTimeMilli = toTime;
	req->IncludeIndicies = (Timestamps!=NULL);
    
    bufferSize += sizeof(DS_RetrieveDataRequest);
            
    
    //////////////////////////////////////////////////////////////////////////
	// (3) Execute
	//////////////////////////////////////////////////////////////////////////
    
    if(Execute((EDS_GlobalState *)global, (EDS_Client *)client, (int)bufferSize) == FALSE)
    {
        DisconnectFromClient(h2, (EDS_Client *)client);
        DisconnectFromGlobal(h1, (EDS_GlobalState *)global);
        mexErrMsgIdAndTxt("MEX:GetBuses:ServerTimedOut", "SERVER TIMED OUT! \n");
    }


	//////////////////////////////////////////////////////////////////////////
	// (3) Populate the data type and actual data
	//////////////////////////////////////////////////////////////////////////

    // IsVariableEncoded: 1 if true, 0 if false
    // SizeInBytes: If variable encoded, this is the size of the ENTIRE blob (all data across all reported timeslices). Otherwise, it's the size of a single element (i.e., entry or value).
    // PointerToTimeStamps: Offset from the base payload to get to the beginning of the timestamps. Set to '-1' if no timestamps are included.
    // PointerToFrameSizes: An array of pointers to get from the base payload to the respective frame size arrays. Always '-1' for non-variable encoded fields. If variable, it will populate up to ElementCount (2 for stereo audio)
    // StartOfData: An array of pointers to the beginning of the actual data. One field per element.
    
    
	//mexPrintf("%s\n", ((EDS_Client *)client)->Result); 
    if(strncmp(((EDS_Client *)client)->Result, "success", 7) == 0)
	{
		resp = (DS_RetrieveDataResponse *)Buffer;
        
        *nTimeslices = resp->FrameCount;

        payload = &resp->StartOfPayload + resp->StartOfData[0];   // Gets a pointer to the start of the element data payload
        
        if (Data != NULL && *nTimeslices > 0 && resp->LoadedSuccessfully[0] > 0)
        {
            switch(resp->Types[0])
            {
                case MappsTypeInt16:
                    if (DataType != NULL)
                        *DataType = "int16";
                    if (BytesPerEntry != NULL)
                        *BytesPerEntry = (int)sizeof(int16);
                    if (resp->IsVariableEncoded == 1)
                    {
                        *Data = (void*)mxCalloc(1, (mwSize)resp->SizeInBytes[0]);
                        memcpy(*Data, payload, (size_t)resp->SizeInBytes[0]);
                        *EntriesPerTimeslice = (int*)mxCalloc((mwSize)*nTimeslices, (mwSize)sizeof(int));
                        memcpy(*EntriesPerTimeslice, &resp->StartOfPayload + resp->PointerToFrameSizes[0], (size_t)((*nTimeslices)*sizeof(int)));
                        for(j=0; j<*nTimeslices; j++)
                        {
                            (*EntriesPerTimeslice)[j] /= (int)sizeof(int16);
                        }
                    }
                    else 
                    {
                        *Data = (void*)mxCalloc((mwSize)*nTimeslices, (mwSize)resp->SizeInBytes[0]);
                        memcpy(*Data, payload, (size_t)((*nTimeslices)*sizeof(int16)));
                        *EntriesPerTimeslice = (int*)mxCalloc(1, (mwSize)sizeof(int));
                        (*EntriesPerTimeslice)[0] = -1;
                    }
                    break;
                
                case MappsTypeInt:
                    if (DataType != NULL)
                        *DataType = "int";
                    if (BytesPerEntry != NULL)
                        *BytesPerEntry = (int)sizeof(int);
                    if (resp->IsVariableEncoded == 1)
                    {
                        *Data = (void*)mxCalloc(1, (mwSize)resp->SizeInBytes[0]);
                        memcpy(*Data, payload, (size_t)resp->SizeInBytes[0]);
                        *EntriesPerTimeslice = (int*)mxCalloc((mwSize)*nTimeslices, (mwSize)sizeof(int));
                        memcpy(*EntriesPerTimeslice, &resp->StartOfPayload + resp->PointerToFrameSizes[0], (size_t)((*nTimeslices)*sizeof(int)));
                        for(j=0; j<*nTimeslices; j++)
                        {
                            (*EntriesPerTimeslice)[j] /= (int)sizeof(int);
                        }
                    }
                    else 
                    {
                        *Data = (void*)mxCalloc((mwSize)*nTimeslices, (mwSize)resp->SizeInBytes[0]);
                        memcpy(*Data, payload, (size_t)((*nTimeslices)*sizeof(int)));
                        *EntriesPerTimeslice = (int*)mxCalloc(1, (mwSize)sizeof(int));
                        (*EntriesPerTimeslice)[0] = -1;
                    }
                    break;

                case MappsTypeInt64:
                    if (DataType != NULL)
                        *DataType = "int64";
                    if (BytesPerEntry != NULL)
                        *BytesPerEntry = (int)sizeof(int64);
                    if (resp->IsVariableEncoded == 1)
                    {
                        *Data = (void*)mxCalloc(1, (mwSize)resp->SizeInBytes[0]);
                        memcpy(*Data, payload, (size_t)resp->SizeInBytes[0]);
                        *EntriesPerTimeslice = (int*)mxCalloc((mwSize)*nTimeslices, (mwSize)sizeof(int));
                        memcpy(*EntriesPerTimeslice, &resp->StartOfPayload + resp->PointerToFrameSizes[0], (size_t)((*nTimeslices)*sizeof(int)));
                        for(j=0; j<*nTimeslices; j++)
                        {
                            (*EntriesPerTimeslice)[j] /= (int)sizeof(int64);
                        }
                    }
                    else
                    {
                        *Data = (void*)mxCalloc((mwSize)*nTimeslices, (mwSize)resp->SizeInBytes[0]);
                        memcpy(*Data, payload, (size_t)((*nTimeslices)*sizeof(int64)));
                        *EntriesPerTimeslice = (int*)mxCalloc(1, (mwSize)sizeof(int));
                        (*EntriesPerTimeslice)[0] = -1;
                    }
                    break;

                case MappsTypeFloat:
                    if (DataType != NULL)
                        *DataType = "float";
                    if (BytesPerEntry != NULL)
                        *BytesPerEntry = (int)sizeof(float);
                    if (resp->IsVariableEncoded == 1)
                    {
                        *Data = (void*)mxCalloc(1, (mwSize)resp->SizeInBytes[0]);
                        memcpy(*Data, payload, (size_t)resp->SizeInBytes[0]);
                        *EntriesPerTimeslice = (int*)mxCalloc((mwSize)*nTimeslices, (mwSize)sizeof(int));
                        memcpy(*EntriesPerTimeslice, &resp->StartOfPayload + resp->PointerToFrameSizes[0], (size_t)((*nTimeslices)*sizeof(int)));
                        for(j=0; j<*nTimeslices; j++)
                        {
                            (*EntriesPerTimeslice)[j] /= (int)sizeof(float);
                        }
                    }
                    else
                    {
                        *Data = (void*)mxCalloc((mwSize)*nTimeslices, (mwSize)resp->SizeInBytes[0]);
                        memcpy(*Data, payload, (size_t)((*nTimeslices)*sizeof(float)));
                        *EntriesPerTimeslice = (int*)mxCalloc(1, (mwSize)sizeof(int));
                        (*EntriesPerTimeslice)[0] = -1;
                    }
                    break;

                case MappsTypeDouble:
                    if (DataType != NULL)
                        *DataType = "double";
                    if (BytesPerEntry != NULL)
                        *BytesPerEntry = (int)sizeof(double);
                    if (resp->IsVariableEncoded == 1)
                    {
                        *Data = (void*)mxCalloc(1, (mwSize)resp->SizeInBytes[0]);
                        memcpy(*Data, payload, (size_t)resp->SizeInBytes[0]);
                        *EntriesPerTimeslice = (int*)mxCalloc((mwSize)*nTimeslices, (mwSize)sizeof(int));
                        memcpy(*EntriesPerTimeslice, &resp->StartOfPayload + resp->PointerToFrameSizes[0], (size_t)((*nTimeslices)*sizeof(int)));
                        for(j=0; j<*nTimeslices; j++)
                        {
                            (*EntriesPerTimeslice)[j] /= (int)sizeof(double);
                        }
                    }
                    else
                    {
                        *Data = (void*)mxCalloc((mwSize)*nTimeslices, (mwSize)resp->SizeInBytes[0]);
                        memcpy(*Data, payload, (size_t)((*nTimeslices)*sizeof(double)));
                        *EntriesPerTimeslice = (int*)mxCalloc(1, (mwSize)sizeof(int));
                        (*EntriesPerTimeslice)[0] = -1;
                    }
                    break;

                case MappsTypeString:
                    if (DataType != NULL)
                        *DataType = "string";

                    /* *Data = (void*)mxCalloc((mwSize)*nTimeslices, (mwSize)sizeof(char*));
                    elementString = (char **)(*Data);
                    for(i=0; i<*nTimeslices; i++)
                    {
                        elementString[i] = (char*)mxCalloc((mwSize)resp->SizeInBytes[0], (mwSize)sizeof(char*));
                        strcpy(elementString[i], payload+i*resp->SizeInBytes);
                    } */

                    // *Data = (void*)mxCalloc((mwSize)*nTimeslices, (mwSize)sizeof(mxArray*));
                    // element_mxString = (mxArray **)(*Data);
                    // for(i=0; i<*nTimeslices; i++)
                    // {
                    // 	element_mxString[i] = mxCreateString(payload+i*resp->SizeInBytes[0]);
                    // }

                    if (resp->IsVariableEncoded == 1)
                    {
                        DisconnectFromClient(h2, (EDS_Client *)client);
                        DisconnectFromGlobal(h1, (EDS_GlobalState *)global);
                        mexErrMsgTxt("Add code for case of variable string lengths per timeslice! \n");
                    }
                    else
                    {
                        if (BytesPerEntry != NULL)
                            *BytesPerEntry = resp->SizeInBytes[0];  // include nul character (???)
                        elementString = (char**)mxCalloc((mwSize)*nTimeslices, (mwSize)sizeof(char*));
                        for(j=0; j<*nTimeslices; j++)
                            elementString[j] = payload+j*resp->SizeInBytes[0];
                        *Data = (void*)mxCreateCharMatrixFromStrings((mwSize)*nTimeslices, elementString);
                        mxFree(elementString);
                        *EntriesPerTimeslice = (int*)mxCalloc(1, (mwSize)sizeof(int));
                        (*EntriesPerTimeslice)[0] = -1;
                    }

                    break;

                default:
                    if (DataType != NULL)
                        *DataType = "unknown";
                    *Data = NULL;
                    break;
            }
        }
        else {
            if (DataType != NULL)
                *DataType = "unknown";
            if (Data != NULL)    
                *Data = NULL;
        }
        
        if(Timestamps != NULL && *nTimeslices > 0)
        {
            //printf("Timestamps requested for bus = %s\n", Busname);
            if (Timestamp_DataType != NULL)
                *Timestamp_DataType = "int64";
            //printf("Timestamp data type = %s and nTimeslices = %llu\n", *Timestamp_DataType, *nTimeslices);
            *Timestamps = (void*)mxCalloc((mwSize)*nTimeslices, (mwSize)sizeof(int64));
            memcpy(*Timestamps, &resp->StartOfPayload, (size_t)((*nTimeslices)*sizeof(int64)));
        }
        else {
            if (Timestamp_DataType != NULL)    
                *Timestamp_DataType = "unknown";
        }
    }
	else
	{
		mexPrintf("Details (mxGetBuses) for bus '%s', field '%s': %s\n", Busname, Fieldname, ((EDS_Client *)client)->Details);
		if (DataType != NULL)
            *DataType = "unknown";  
		if (Data != NULL)
            *Data = NULL;
        if (Timestamp_DataType != NULL)
            *Timestamp_DataType = "unknown";
        if (Timestamps != NULL)
            *Timestamps = NULL;
	}
}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) 
{
#include "_SetUpConnection_global_declaration.h"
#include "_SetUpConnection_client_declaration.h"
    
    void *Buffer;
    
    char MAPPS_TIME_STRING[] = "MAPPS_time";
    
    const mxArray *FieldsRequested_ptr, *AssocBuses_ptr, *StructRequested_ptr, *BusStructRequested_ptr;
    mxArray *DataStruct_ptr, *Name_ptr, *Data_ptr, *Struct_ptr;
    char *Field_string, *Bus_string, *MAPPS_DataType, *MAPPS_Timestamp_DataType;
    char **Buses_ptr, *EncodedVariableFields_ptr[2];
    mwSize dims[2];
    int *BusID, *MAPPS_EntriesPerTimeslice;
    uint64 *StartIndices;
    void *MAPPS_Data, *MAPPS_Timestamps;
    mwIndex i, j, maxID;
    size_t nRequested, name_length, nFields;
    mwSize nBuses;
    int unique, fieldnumber, id, MAPPS_BytesPerEntry;
    uint64 nTimeslices, nTimeslices_total;
    int64 fromTime_MAPPS, toTime_MAPPS;
    mxClassID category;
    
    
    //////////////////////////////////////////////////////////////////////////
	// 1. Check for proper number of input and output arguments
	//////////////////////////////////////////////////////////////////////////
    
    if (!(nrhs == 2 || nrhs == 3 || nrhs == 4 || nrhs == 5))
        mexErrMsgTxt("Input argument combinations allowed are [struct, struct], or [struct, struct, scalar, scalar], [struct, cellstr, cellstr], or [struct, cellstr, cellstr, scalar, scalar].\n");
    
    if (!(!(mxIsEmpty(prhs[0])) && mxIsStruct(prhs[0])))
        mexErrMsgTxt("First input argument is not a struct.\n");
    
    if (nrhs == 2 || nrhs == 4) 
    {
        if (!(!(mxIsEmpty(prhs[1])) && mxIsStruct(prhs[1])))
            mexErrMsgTxt("Second input argument is not a struct.\n");
    }
    
    if (nrhs == 3 || nrhs == 5) 
    {
        if (!(mxIsEmpty(prhs[1]))) 
        {
            if (!(mxIsCell(prhs[1]) && mxGetNumberOfDimensions(prhs[1]) == 2))
                mexErrMsgTxt("Second input argument is not a cell array.\n");
            else {
                if (mxGetM(prhs[1]) != 1 && mxGetN(prhs[1]) != 1)
                    mexErrMsgTxt("Second input argument is not a vector.\n");
            }
        }
        if (!(mxIsCell(prhs[2]) &&  mxGetNumberOfDimensions(prhs[2]) == 2))
            mexErrMsgTxt("Third input argument is not a cell array.\n");
        else {
            if (mxGetM(prhs[2]) != 1 && mxGetN(prhs[2]) != 1) {
                mexErrMsgTxt("Third input argument is not a vector.\n");
            }    
        }
    }
    
    if (nrhs == 4)
    {
        if (!(!(mxIsEmpty(prhs[2])) && mxIsNumeric(prhs[2]) && mxGetNumberOfDimensions(prhs[2]) == 2 && mxGetM(prhs[2]) == 1 && mxGetN(prhs[2]) == 1))
            mexErrMsgTxt("Third input argument is not a scalar.\n");
        if (!(!(mxIsEmpty(prhs[3])) && mxIsNumeric(prhs[3]) && mxGetNumberOfDimensions(prhs[3]) == 2 && mxGetM(prhs[3]) == 1 && mxGetN(prhs[3]) == 1))
            mexErrMsgTxt("Fourth input argument is not a scalar.\n");
    }
    
    if (nrhs == 5)
    {
        if (!(!(mxIsEmpty(prhs[3])) && mxIsNumeric(prhs[3]) && mxGetNumberOfDimensions(prhs[3]) == 2 && mxGetM(prhs[3]) == 1 && mxGetN(prhs[3]) == 1))
            mexErrMsgTxt("Fourth input argument is not a scalar.\n");
        if (!(!(mxIsEmpty(prhs[4])) && mxIsNumeric(prhs[4]) && mxGetNumberOfDimensions(prhs[4]) == 2 && mxGetM(prhs[4]) == 1 && mxGetN(prhs[4]) == 1))
            mexErrMsgTxt("Fifth input argument is not a scalar.\n");
    }
    
    if(nlhs != 1){
        mexErrMsgTxt("A single output argument is required.\n");
    }


    //////////////////////////////////////////////////////////////////////////
	// 2. Extract data passed from Matlab
	////////////////////////////////////////////////////////////////////////// 
    
    if (nrhs == 4) {
        fromTime_MAPPS = (int64)mxGetScalar(prhs[2]);
        toTime_MAPPS = (int64)mxGetScalar(prhs[3]);
    }
    else if (nrhs == 5) {
        fromTime_MAPPS = (int64)mxGetScalar(prhs[3]);
        toTime_MAPPS = (int64)mxGetScalar(prhs[4]);
    }
    else {
        fromTime_MAPPS = 0;
        toTime_MAPPS = 0;
    }
    
    if (nrhs == 2 || nrhs == 4) {
        StructRequested_ptr = prhs[1];
        nBuses = (mwSize)mxGetNumberOfFields(StructRequested_ptr);
        nRequested = 0;
        for (i = 0; i < (mwIndex)nBuses; i++) {
            BusStructRequested_ptr = mxGetFieldByNumber(StructRequested_ptr, 0, (int)i);
            if (BusStructRequested_ptr != NULL && mxIsStruct(BusStructRequested_ptr))
                nFields = (size_t)mxGetNumberOfFields(BusStructRequested_ptr);
            else
                nFields = 0;
            if (nFields == 0)
                nFields = 1;
            nRequested = nRequested + nFields;
        }
        Buses_ptr = (char **)mxCalloc(nRequested, sizeof(char *));
        BusID = (int *)mxCalloc(nRequested, sizeof(int));
        id = 0;
        for (i = 0; i < (mwIndex)nBuses; i++) {
            Bus_string = (char *)mxGetFieldNameByNumber(StructRequested_ptr, (int)i);
            name_length = strlen(Bus_string) + 1;
            Buses_ptr[i] = (char *)mxCalloc((mwSize)name_length, sizeof(char));
            strcpy(Buses_ptr[i], Bus_string);
            
            BusStructRequested_ptr = mxGetFieldByNumber(StructRequested_ptr, 0, (int)i);
            if (BusStructRequested_ptr != NULL && mxIsStruct(BusStructRequested_ptr))
                nFields = (size_t)mxGetNumberOfFields(BusStructRequested_ptr);
            else
                nFields = 0;
            if (nFields == 0) {
                BusID[id] = (int)i;
                id++;
            }
            else {
                for (j = 0; j < (mwIndex)nFields; j++) {
                    BusID[id] = (int)i;
                    id++;
                }
            }
        }
    }
    else {
        FieldsRequested_ptr = prhs[1];
        AssocBuses_ptr = prhs[2];
        nRequested = mxGetNumberOfElements(AssocBuses_ptr);
        if (!(mxIsEmpty(FieldsRequested_ptr)) && mxGetNumberOfElements(FieldsRequested_ptr) != nRequested)
            mexErrMsgIdAndTxt("MEX:GetBuses:NotMatched", "Input arguments do not have the same number of elements!");
        nBuses = 0;
        Buses_ptr = (char **)mxCalloc(nRequested, sizeof(char *));
        BusID = (int *)mxCalloc(nRequested, sizeof(int));
        for (i = 0; i < (mwIndex)nRequested; i++) {
            Name_ptr = mxGetCell(AssocBuses_ptr, i);
            name_length = mxGetNumberOfElements(Name_ptr) + 1;
            Bus_string = (char *)mxCalloc((mwSize)name_length, sizeof(char));
            if (mxGetString(Name_ptr, Bus_string, (mwSize)name_length) != 0)
                mexErrMsgIdAndTxt("MEX:GetBuses:ConversionError", "Could not convert string data for associated bus name in %dth cell of input argument!", i);
            unique = 1;
            if (i > 0) {
                for (j = 0; j < nBuses; j++) {
                    if (unique == 1 && strcmp((const char *)Bus_string, (const char *)Buses_ptr[j]) == 0) {
                        unique = 0;
                        BusID[i] = (int)j;
                    }
                }
            }
            if (unique == 1) {
                Buses_ptr[nBuses] = Bus_string;
                BusID[i] = (int)nBuses;
                nBuses++;
            }
            else {
                mxFree(Bus_string);
            }    
        }
    }
   
    /*mexPrintf("Total number of unique buses in request = %d\n", nBuses);
    mexPrintf("Bus IDs =");
    for (i = 0; i < (mwIndex)nRequested; i++) {
        mexPrintf(" %d", BusID[i]);
    }
    mexPrintf(".\n");*/
    
    dims[0] = 1;
    dims[1] = 1;
    DataStruct_ptr = mxCreateStructArray(2, dims, (int)nBuses, (const char **)Buses_ptr);
    Field_string = MAPPS_TIME_STRING;
    for (i = 0; i < nBuses; i++) {
        dims[0] = 1;
        dims[1] = 1;
        mxSetField(DataStruct_ptr, 0, (const char *)Buses_ptr[i], mxCreateStructArray(2, dims, 1, (const char **)&Field_string));
    }
    
    
    //////////////////////////////////////////////////////////////////////////
	// 3. Connect to the MAPPS shared memory buffer
	//////////////////////////////////////////////////////////////////////////    
    
#include "_SetUpConnection_global_code.h"
#include "_SetUpConnection_client_code.h"
    
    Buffer = ConnectToClient(&h2, client);
	
	if(Buffer == NULL)
	{
		DisconnectFromGlobal(h1, global);
		mexErrMsgIdAndTxt("MEX:GetBuses:UnableToConnectClient", "Unable to connect to shared memory buffer (client state)! \n");
	}
		

    //////////////////////////////////////////////////////////////////////////
	// 4. Populate
	//////////////////////////////////////////////////////////////////////////

    if (RETRIEVE_SINGLE_FIELD_AT_A_TIME == 1) {
        if (nrhs == 2 || nrhs == 4) {
            fieldnumber = 0;
        }
        else {
            Field_string = (char *)mxCalloc(1024, sizeof(char));
        }
        maxID = 0;
        
        EncodedVariableFields_ptr[0] = "Data";
        EncodedVariableFields_ptr[1] = "StartIndices";
        for (i = 0; i < (mwIndex)nRequested; i++) {
            Bus_string = Buses_ptr[BusID[i]];

            if (nrhs == 2 || nrhs == 4) {
                BusStructRequested_ptr = mxGetFieldByNumber(StructRequested_ptr, 0, BusID[i]);
                if (BusStructRequested_ptr != NULL && mxIsStruct(BusStructRequested_ptr))
                    Field_string = (char *)mxGetFieldNameByNumber(BusStructRequested_ptr, fieldnumber);
                else
                    Field_string == NULL;
                if (Field_string == NULL || (i + 1 < (mwIndex)nRequested && BusID[i] != BusID[i + 1])) {
                    fieldnumber = 0;
                }
                else {
                    fieldnumber++;
                }
            }
            else {
                if (!(mxIsEmpty(FieldsRequested_ptr))) {
                    Name_ptr = mxGetCell(FieldsRequested_ptr, i);
                    name_length = mxGetNumberOfElements(Name_ptr) + 1;
                    if (mxGetString(Name_ptr, Field_string, (mwSize)name_length) != 0) {
                        DisconnectFromClient(h2, client);
                        DisconnectFromGlobal(h1, global);
                        mexErrMsgIdAndTxt("MEX:GetBuses:ConversionError", "Could not convert string data for field name in %dth cell of input argument!", i);
                    }
                }
                else {
                    strcpy(Field_string, "");
                }
            }

            /* 1) Search for field "Field_string" in the MAPPS buffer "Bus_string"
             * 2) If available, copy elements between "fromTime_MAPPS" and "toTime_MAPPS" 
             *    into vector, and pass back in "MAPPS_Data"
             * 3) Report data type (single, double, uint64, char, etc.) in "MAPPS_DataType"
             * 4) If *Timestamps != NULL (i.e., associated timestamps requested), 
             *    copy MAPPS timestamps between "fromTime_MAPPS" and "toTime_MAPPS" into vector, 
             *    and pass back in "MAPPS_Timestamps"
             * 5) If *Timestamps != NULL (i.e., associated timestamps requested), 
             *    report timestamp data type (single, double, uint64, char, etc.) in "MAPPS_Timestamp_DataType"
             * 6) Report number of timeslices in "nTimeslices" */

            if (Field_string == NULL || mxGetFieldNumber(mxGetField(DataStruct_ptr, 0, (const char *)Bus_string), (const char *)Field_string) < 0) {
                if (i == 0 || BusID[i] > maxID) {
                    Retrieve_MAPPS_Data_Single_Field((const char *)Bus_string, (const char *)Field_string, fromTime_MAPPS, toTime_MAPPS, 
                                                     &MAPPS_Data, &MAPPS_DataType, &MAPPS_BytesPerEntry, &MAPPS_EntriesPerTimeslice, 
                                                     &MAPPS_Timestamps, &MAPPS_Timestamp_DataType, &nTimeslices,
                                                     (void *)global, (void *)client, h1, h2, Buffer);
                    maxID = BusID[i];
                }
                else {
                    Retrieve_MAPPS_Data_Single_Field((const char *)Bus_string, (const char *)Field_string, fromTime_MAPPS, toTime_MAPPS, 
                                                     &MAPPS_Data, &MAPPS_DataType, &MAPPS_BytesPerEntry, &MAPPS_EntriesPerTimeslice, 
                                                     NULL, NULL, &nTimeslices,
                                                     (void *)global, (void *)client, h1, h2, Buffer);
                    MAPPS_Timestamps = NULL;
                    MAPPS_Timestamp_DataType = NULL;
                }
                
                //mexPrintf("Data type = %s and Timestamp data type = %s\n", MAPPS_DataType, MAPPS_Timestamp_DataType);
                if (MAPPS_Timestamps != NULL && nTimeslices > 0) {
                    category = Interpret_MAPPS_DataType(MAPPS_Timestamp_DataType);

                    dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, MAPPS_Data)
                    dims[1] = 1;
                    Data_ptr = mxCreateNumericArray(2, dims, category, mxREAL);
                    mxSetM(Data_ptr, (mwSize)nTimeslices);
                    mxSetData(Data_ptr, MAPPS_Timestamps);
                    mxSetField(mxGetField(DataStruct_ptr, 0, (const char *)Bus_string), 0, MAPPS_TIME_STRING, Data_ptr);
                }
                if (MAPPS_Data != NULL && nTimeslices > 0) {
                    mxAddField(mxGetField(DataStruct_ptr, 0, (const char *)Bus_string), Field_string);

                    category = Interpret_MAPPS_DataType(MAPPS_DataType);

                    if (category != mxCHAR_CLASS) {
                        if (*MAPPS_EntriesPerTimeslice != -1) {                       
                            dims[0] = 1;
                            dims[1] = 1;
                            Struct_ptr = mxCreateStructArray(2, dims, 2, (const char **)EncodedVariableFields_ptr);
                            
                            StartIndices = (uint64 *)mxCalloc((mwSize)nTimeslices, (mwSize)sizeof(uint64));
                            StartIndices[0] = 1;
                            nTimeslices_total = MAPPS_EntriesPerTimeslice[0];
                            for (j = 1; j < nTimeslices; j++) {
                                StartIndices[j] = 1 + nTimeslices_total;
                                nTimeslices_total += (uint64)MAPPS_EntriesPerTimeslice[j];
                            }
                            dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, MAPPS_Data)
                            dims[1] = 1;
                            Data_ptr = mxCreateNumericArray(2, dims, category, mxREAL);
                            mxSetM(Data_ptr, (mwSize)nTimeslices_total);
                            mxSetData(Data_ptr, MAPPS_Data);
                            mxSetField(Struct_ptr, 0, (const char *)EncodedVariableFields_ptr[0], Data_ptr);
                            dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, MAPPS_Data)
                            dims[1] = 1;
                            Data_ptr = mxCreateNumericArray(2, dims, mxUINT64_CLASS, mxREAL);
                            mxSetM(Data_ptr, (mwSize)nTimeslices);
                            mxSetData(Data_ptr, StartIndices);
                            mxSetField(Struct_ptr, 0, (const char *)EncodedVariableFields_ptr[1], Data_ptr);
                            
                            mxSetField(mxGetField(DataStruct_ptr, 0, (const char *)Bus_string), 0, (const char *)Field_string, Struct_ptr);
                        }
                        else {
                            dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, MAPPS_Data)
                            dims[1] = 1;
                            Data_ptr = mxCreateNumericArray(2, dims, category, mxREAL);
                            mxSetM(Data_ptr, (mwSize)nTimeslices);
                            mxSetData(Data_ptr, MAPPS_Data);  
                            mxSetField(mxGetField(DataStruct_ptr, 0, (const char *)Bus_string), 0, (const char *)Field_string, Data_ptr);           
                        }
                    }
                    else {
                        /* String_ptr = (char **)MAPPS_Data;
                        dims[0] = (mwSize)nTimeslices;
                        dims[1] = 1;
                        Cell_ptr = mxCreateCellArray(2, dims);
                        for (j = 0; j < nTimeslices; j++) {
                            mxSetCell(Cell_ptr, (mwIndex)j, mxCreateString(String_ptr[j]));
                            mxFree(String_ptr[j]);
                        }
                        mxSetField(mxGetField(DataStruct_ptr, 0, (const char *)Bus_string), 0, (const char *)Field_string, Cell_ptr);
                        mxFree(MAPPS_Data); */

                        // mxArray_ptr = (mxArray **)MAPPS_Data;
                        // dims[0] = (mwSize)nTimeslices;
                        // dims[1] = 1;
                        // Cell_ptr = mxCreateCellArray(2, dims);
                        // for (j = 0; j < nTimeslices; j++) {
                        //     mxSetCell(Cell_ptr, (mwIndex)j, mxArray_ptr[j]);
                        // }
                        // mxSetField(mxGetField(DataStruct_ptr, 0, (const char *)Bus_string), 0, (const char *)Field_string, Cell_ptr);
                        // mxFree(MAPPS_Data);

                        mxSetField(mxGetField(DataStruct_ptr, 0, (const char *)Bus_string), 0, (const char *)Field_string, (mxArray *)MAPPS_Data);                    
                    }
                    mxFree(MAPPS_EntriesPerTimeslice);
                }
            }
        }
    }
    
    plhs[0] = DataStruct_ptr;
    
    
    //////////////////////////////////////////////////////////////////////////
	// 5. Disconnect from the MAPPS shared memory buffer
	//////////////////////////////////////////////////////////////////////////
	
	DisconnectFromClient(h2, client);
    DisconnectFromGlobal(h1, global);
    
    
    //////////////////////////////////////////////////////////////////////////
	// 6. Free allocated memory
	//////////////////////////////////////////////////////////////////////////
    
    for (i = 0; i < (mwIndex)nRequested; i++) {
        mxFree(Buses_ptr[i]);
    }
    mxFree(Buses_ptr);
    mxFree(BusID);
    
    if (nrhs == 3 || nrhs == 5) {
        mxFree(Field_string);
    }
}
