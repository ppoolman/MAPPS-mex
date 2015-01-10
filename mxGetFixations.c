/*=================================================================
 * mxGetFixations.c 
 *
 *=================================================================*/

#include "mxDataIO.h"

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
#include "_SetUpConnection_global_declaration.h"
#include "_SetUpConnection_client_declaration.h"
    
    size_t bufferSize;
    void *Buffer; 
    DS_RetrieveFixationRequest *req;
    DS_Fixations *resp;
    
	int i, nF;
    int *CanvasIndex, *CanvasIndex_value;
    int64 *From_MAPPS_time, *To_MAPPS_time, *From_MAPPS_time_value, *To_MAPPS_time_value;
    float *MidPointX, *MidPointY, *MidPointX_value, *MidPointY_value;
    char *StructFieldNames_ptr[5];
    mxArray *FixationsStruct_ptr, *Data_ptr;
    mwSize dims[2];
    DS_FixationEntry *Fixation;
	
    
    //////////////////////////////////////////////////////////////////////////
	// 1. Check for proper number of input and output arguments
	//////////////////////////////////////////////////////////////////////////
    
    /* prhs[1] <= subject index
     * prhs[2] <= start time of requested interval
     * prhs[3] <= end time of requested inteval
     * plhs[0] <= struct with fixation details provided */
    
    if (!(nrhs == 2 || nrhs == 4))
        mexErrMsgTxt("Input argument combinations allowed are [struct, scalar], or [struct, scalar, scalar, scalar].\n");
    
    if (!(!(mxIsEmpty(prhs[0])) && mxIsStruct(prhs[0])))
        mexErrMsgTxt("First input argument is not a struct.\n");
    
    if (!(!(mxIsEmpty(prhs[1])) && mxIsNumeric(prhs[1]) && mxGetNumberOfDimensions(prhs[1]) == 2 && mxGetM(prhs[1]) == 1 && mxGetN(prhs[1]) == 1 && mxGetScalar(prhs[1]) > 0))
        mexErrMsgTxt("Second input argument is not a positive scalar.\n");
    
    if (nrhs == 4)
    {
        if (!(!(mxIsEmpty(prhs[2])) && mxIsNumeric(prhs[2]) && mxGetNumberOfDimensions(prhs[2]) == 2 && mxGetM(prhs[2]) == 1 && mxGetN(prhs[2]) == 1))
            mexErrMsgTxt("Third input argument is not a scalar.\n");
        if (!(!(mxIsEmpty(prhs[3])) && mxIsNumeric(prhs[3]) && mxGetNumberOfDimensions(prhs[3]) == 2 && mxGetM(prhs[3]) == 1 && mxGetN(prhs[3]) == 1))
            mexErrMsgTxt("Fourth input argument is not a scalar.\n");
    }
       
    if(nlhs > 0 && nlhs != 1)
        mexErrMsgTxt("A single output argument is required.\n");


    //////////////////////////////////////////////////////////////////////////
	// 2. Connect to the MAPPS shared memory buffer
	//////////////////////////////////////////////////////////////////////////    
    
#include "_SetUpConnection_global_code.h"
#include "_SetUpConnection_client_code.h"
    
    Buffer = ConnectToClient(&h2, client);
	
	if(Buffer == NULL)
	{
		DisconnectFromGlobal(h1, global);
		mexErrMsgIdAndTxt("MEX:GetFixations:UnableToConnectClient", "Unable to connect to shared memory buffer (client state)! \n");
	}
		
		
	//////////////////////////////////////////////////////////////////////////
	// 3. Prepare the buffer for use
	//////////////////////////////////////////////////////////////////////////
	
	if(Prepare(global, client, EDS_GetFixations) == FALSE)
	{
		DisconnectFromClient(h2, client);
        DisconnectFromGlobal(h1, global);
		mexErrMsgIdAndTxt("MEX:GetFixations:ClientBlocked", "Server is still blocking the client channel! \n");
	}


    //////////////////////////////////////////////////////////////////////////
	// 4. Populate
	//////////////////////////////////////////////////////////////////////////

	req = (DS_RetrieveFixationRequest *)Buffer;
    bufferSize = 0;
    
    req->SubjectIndex = (int)mxGetScalar(prhs[1]) - 1;
    if (nrhs == 4) {
        req->StartTimeMilli = (int64)mxGetScalar(prhs[2]);
        req->EndTimeMilli = (int64)mxGetScalar(prhs[3]);
    } 
    else {
        req->StartTimeMilli = 0;
        req->EndTimeMilli = 0;
    }
    
    bufferSize += sizeof(DS_RetrieveFixationRequest);


	//////////////////////////////////////////////////////////////////////////
	// 5. Execute
	//////////////////////////////////////////////////////////////////////////
	
    if(Execute(global, client, (int)bufferSize) == FALSE)
    {
        DisconnectFromClient(h2, client);
        DisconnectFromGlobal(h1, global);
        mexErrMsgIdAndTxt("MEX:GetFixations:ServerTimedOut", "SERVER TIMED OUT! \n");
    }

	
    //////////////////////////////////////////////////////////////////////////
	// 6. Echo the result
	//////////////////////////////////////////////////////////////////////////

	if (strncmp(client->Result, "success", 7) == 0) 
    {
		resp = (DS_Fixations*)Buffer;
        if (nlhs == 0) {
            mexPrintf("There %s %d %s \n", 
                      resp->Count == 1 ? "is" : "are",
                      resp->Count, 
                      resp->Count == 1 ? "fixation:" : "fixations:");

            for(i=0; i<resp->Count; i++)
            {
                printf("   [%3d] Canvas: %d.  Time: (%3.3f-%3.3f).  XY: (%1.3f,%1.3f) \n", 
                    i+1, 
                    resp->Entry[i].DisplayIndex,
                    (float)resp->Entry[i].TimeStartMilli*0.001f,
                    (float)resp->Entry[i].TimeEndMilli*0.001f,
                    resp->Entry[i].MidPointX,
                    resp->Entry[i].MidPointY);
            }
        }
	}
	else
	{
		mexPrintf("Details (mxGetFixations): %s\n", client->Details);
	}


    //////////////////////////////////////////////////////////////////////////
	// 7. Populate Matlab output
	//////////////////////////////////////////////////////////////////////////
    
    if (nlhs == 1) {
        if (strncmp(client->Result, "success", 7) == 0) {        
            nF = resp->Count;

            CanvasIndex = (int *)mxCalloc((mwSize)nF, (mwSize)sizeof(int));
            From_MAPPS_time = (int64 *)mxCalloc((mwSize)nF, (mwSize)sizeof(int64));
            To_MAPPS_time = (int64 *)mxCalloc((mwSize)nF, (mwSize)sizeof(int64));
            MidPointX = (float *)mxCalloc((mwSize)nF, (mwSize)sizeof(float));
            MidPointY = (float *)mxCalloc((mwSize)nF, (mwSize)sizeof(float));

            Fixation = (DS_FixationEntry *)&(resp->Entry[0]);
            CanvasIndex_value = CanvasIndex;
            From_MAPPS_time_value = From_MAPPS_time;
            To_MAPPS_time_value = To_MAPPS_time;
            MidPointX_value = MidPointX;
            MidPointY_value = MidPointY;
            for (i = 0; i < nF; i++) {
                /**CanvasIndex_value++ = resp->Entry[i].DisplayIndex;
                *From_MAPPS_time_value++ = resp->Entry[i].TimeStartMilli;
                *To_MAPPS_time_value++ = resp->Entry[i].TimeEndMilli;
                *MidPointX_value++ = resp->Entry[i].MidPointX;
                *MidPointY_value++ = resp->Entry[i].MidPointY;
                */
                *CanvasIndex_value++ = Fixation->DisplayIndex;
                *From_MAPPS_time_value++ = Fixation->TimeStartMilli;
                *To_MAPPS_time_value++ = Fixation->TimeEndMilli;
                *MidPointX_value++ = Fixation->MidPointX;
                *MidPointY_value++ = Fixation->MidPointY;
                Fixation++;
            }

            StructFieldNames_ptr[0] = "CanvasIndex";       // int
            StructFieldNames_ptr[1] = "From_MAPPS_time";    // int64
            StructFieldNames_ptr[2] = "To_MAPPS_time";      // int64
            StructFieldNames_ptr[3] = "MidPointX";          // float
            StructFieldNames_ptr[4] = "MidPointY";          // float
            dims[0] = 1;
            dims[1] = 1;
            FixationsStruct_ptr = mxCreateStructArray(2, dims, 5, (const char **)StructFieldNames_ptr);

            dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
            dims[1] = 1;
            Data_ptr = mxCreateNumericArray(2, dims, mxINT32_CLASS, mxREAL);
            mxSetM(Data_ptr, (mwSize)nF);
            mxSetData(Data_ptr, CanvasIndex); 
            mxSetField(FixationsStruct_ptr, 0, StructFieldNames_ptr[0], Data_ptr);      // "CanvasIndex"

            dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
            dims[1] = 1;
            Data_ptr = mxCreateNumericArray(2, dims, mxINT64_CLASS, mxREAL);
            mxSetM(Data_ptr, (mwSize)nF);
            mxSetData(Data_ptr, From_MAPPS_time); 
            mxSetField(FixationsStruct_ptr, 0, StructFieldNames_ptr[1], Data_ptr);      // "From_MAPPS_time"

            dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
            dims[1] = 1;
            Data_ptr = mxCreateNumericArray(2, dims, mxINT64_CLASS, mxREAL);
            mxSetM(Data_ptr, (mwSize)nF);
            mxSetData(Data_ptr, To_MAPPS_time); 
            mxSetField(FixationsStruct_ptr, 0, StructFieldNames_ptr[2], Data_ptr);      // "To_MAPPS_time"

            dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
            dims[1] = 1;
            Data_ptr = mxCreateNumericArray(2, dims, mxSINGLE_CLASS, mxREAL);
            mxSetM(Data_ptr, (mwSize)nF);
            mxSetData(Data_ptr, MidPointX); 
            mxSetField(FixationsStruct_ptr, 0, StructFieldNames_ptr[3], Data_ptr);      // "MidPointX"

            dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
            dims[1] = 1;
            Data_ptr = mxCreateNumericArray(2, dims, mxSINGLE_CLASS, mxREAL);
            mxSetM(Data_ptr, (mwSize)nF);
            mxSetData(Data_ptr, MidPointY); 
            mxSetField(FixationsStruct_ptr, 0, StructFieldNames_ptr[4], Data_ptr);      // "MidPointY"

            plhs[0] = FixationsStruct_ptr;
        }
        else {
            dims[0] = 0;
            dims[1] = 1;
            plhs[0] = mxCreateNumericArray(2, dims, mxDOUBLE_CLASS, mxREAL);
        }
    }


	//////////////////////////////////////////////////////////////////////////
	// 8. Close shared memory handles
	//////////////////////////////////////////////////////////////////////////

    DisconnectFromClient(h2, client);
    DisconnectFromGlobal(h1, global);
}
