/*=================================================================
 * mxGetRoiIntersections.c 
 *
 *=================================================================*/

#include "mxDataIO.h"

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
#include "_SetUpConnection_global_declaration.h"
#include "_SetUpConnection_client_declaration.h"
    
    size_t bufferSize;
    void *Buffer; 
    DS_RoiIntersections *req, *resp;
    
	int i, nT;
    int64 *MAPPS_time, *MAPPS_time_value;
    int *Index, *Index_value;
    char *StructFieldNames_ptr[3];
    char **Names, **Name_ptr;
    mxArray *RoiIntersectionsStruct_ptr, *Data_ptr;
    mwSize dims[2];
    DS_RoiIntersection *RoiIntersection;
	
    
    //////////////////////////////////////////////////////////////////////////
	// 1. Check for proper number of input and output arguments
	//////////////////////////////////////////////////////////////////////////
    
    /* prhs[1] <= subject index
     * prhs[2] <= start time of requested interval
     * prhs[3] <= end time of requested inteval
     * plhs[0] <= struct with ROI intersection details provided */
    
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
		mexErrMsgIdAndTxt("MEX:GetRoiIntersections:UnableToConnectClient", "Unable to connect to shared memory buffer (client state)! \n");
	}
		
		
	//////////////////////////////////////////////////////////////////////////
	// 3. Prepare the buffer for use
	//////////////////////////////////////////////////////////////////////////
	
	if(Prepare(global, client, EDS_GetRoiIntersections) == FALSE)
	{
		DisconnectFromClient(h2, client);
        DisconnectFromGlobal(h1, global);
		mexErrMsgIdAndTxt("MEX:GetRoiIntersections:ClientBlocked", "Server is still blocking the client channel! \n");
	}


    //////////////////////////////////////////////////////////////////////////
	// 4. Populate
	//////////////////////////////////////////////////////////////////////////

	req = (DS_RoiIntersections *)Buffer;
    bufferSize = 0;
    
    req->ComputeAs3D = 0;
    req->SubjectIndex = (int)mxGetScalar(prhs[1]) - 1;
    if (nrhs == 4) {
        req->StartTime = (int64)mxGetScalar(prhs[2]);
        req->EndTime = (int64)mxGetScalar(prhs[3]);
    } 
    else {
        req->StartTime = 0;
        req->EndTime = 0;
    }
    
    bufferSize += sizeof(DS_RoiIntersections);


	//////////////////////////////////////////////////////////////////////////
	// 5. Execute
	//////////////////////////////////////////////////////////////////////////
	
    if(Execute(global, client, (int)bufferSize) == FALSE)
    {
        DisconnectFromClient(h2, client);
        DisconnectFromGlobal(h1, global);
        mexErrMsgIdAndTxt("MEX:GetRoiIntersections:ServerTimedOut", "SERVER TIMED OUT! \n");
    }

	
    //////////////////////////////////////////////////////////////////////////
	// 6. Echo the result
	//////////////////////////////////////////////////////////////////////////

	if (strncmp(client->Result, "success", 7) == 0) 
    {
		resp = (DS_RoiIntersections *)Buffer;
        if (nlhs == 0) {
            mexPrintf("There %s %d %s \n", 
                      resp->FrameCount == 1 ? "is" : "are",
                      resp->FrameCount, 
                      resp->FrameCount == 1 ? "ROI intersection:" : "ROI intersections:");

            for(i=0; i<resp->FrameCount; i++)
            {
                mexPrintf("   [%3d]  Time: %3.3f  Index: %3d  Name: %s \n", 
                          i+1, 
                          (float)resp->Intersections[i].Time*0.001f,
                          resp->Intersections[i].Index,
                          resp->Intersections[i].Name);
            }
        }
	}
	else
	{
		mexPrintf("Details (mxGetRoiIntersections): %s\n", client->Details);
	}


    //////////////////////////////////////////////////////////////////////////
	// 7. Populate Matlab output
	//////////////////////////////////////////////////////////////////////////
    
    if (nlhs == 1) {
        if (strncmp(client->Result, "success", 7) == 0) {        
            nT = resp->FrameCount;

            MAPPS_time = (int64 *)mxCalloc((mwSize)nT, (mwSize)sizeof(int64));
            Index = (int *)mxCalloc((mwSize)nT, (mwSize)sizeof(int));
            Names = (char **)mxCalloc((mwSize)nT, (mwSize)sizeof(char *));

            RoiIntersection = (DS_RoiIntersection *)&(resp->Intersections[0]);            
            MAPPS_time_value = MAPPS_time;
            Index_value = Index;
            Name_ptr = Names;
            for (i = 0; i < nT; i++) {
                *MAPPS_time_value++ = RoiIntersection->Time;
                *Index_value++ = RoiIntersection->Index + 1;
                *Name_ptr++ = RoiIntersection->Name;
                RoiIntersection++;
            }

            StructFieldNames_ptr[0] = "MAPPS_time";     // int64
            StructFieldNames_ptr[1] = "Index";          // int
            StructFieldNames_ptr[2] = "Name";           // char *
            dims[0] = 1;
            dims[1] = 1;
            RoiIntersectionsStruct_ptr = mxCreateStructArray(2, dims, 3, (const char **)StructFieldNames_ptr);

            dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
            dims[1] = 1;
            Data_ptr = mxCreateNumericArray(2, dims, mxINT64_CLASS, mxREAL);
            mxSetM(Data_ptr, (mwSize)nT);
            mxSetData(Data_ptr, MAPPS_time); 
            mxSetField(RoiIntersectionsStruct_ptr, 0, StructFieldNames_ptr[0], Data_ptr);      // "MAPPS_time"
            
            dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
            dims[1] = 1;
            Data_ptr = mxCreateNumericArray(2, dims, mxINT32_CLASS, mxREAL);
            mxSetM(Data_ptr, (mwSize)nT);
            mxSetData(Data_ptr, Index); 
            mxSetField(RoiIntersectionsStruct_ptr, 0, StructFieldNames_ptr[1], Data_ptr);      // "Index"

            Data_ptr = mxCreateCharMatrixFromStrings((mwSize)nT, Names);
            mxSetField(RoiIntersectionsStruct_ptr, 0, StructFieldNames_ptr[2], Data_ptr);      // "Name"

            plhs[0] = RoiIntersectionsStruct_ptr;
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
