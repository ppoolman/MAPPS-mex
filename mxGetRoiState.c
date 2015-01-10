/*=================================================================
 * mxGetRoiState.c 
 *
 *=================================================================*/

#include "mxDataIO.h"

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
#include "_SetUpConnection_global_declaration.h"
#include "_SetUpConnection_client_declaration.h"
    
    size_t bufferSize;
    void *Buffer;
    DS_RoiState *resp;    
    
    char *StateFieldNames_ptr[3]; 
    int *IntValue_ptr;
    int64 *Int64Value_ptr;
    mxArray *StateStruct_ptr, *Data_ptr;
    mwSize dims[2];
    
    
    //////////////////////////////////////////////////////////////////////////
	// 1. Check for proper number of input and output arguments
	//////////////////////////////////////////////////////////////////////////

    if (!(nrhs == 1))
        mexErrMsgTxt("Input argument combination allowed is [struct].\n");
    
    if (!(!(mxIsEmpty(prhs[0])) && mxIsStruct(prhs[0])))
        mexErrMsgTxt("First input argument is not a struct.\n"); 
    
    if (!(nlhs == 0 || nlhs == 1)){
        mexErrMsgTxt("No more than a single output argument is required.\n");
    }
    
    
    //////////////////////////////////////////////////////////////////////////
	// 2. Connect to the MAPPS shared memory buffer
	//////////////////////////////////////////////////////////////////////////    
    
#include "_SetUpConnection_global_code.h"
#include "_SetUpConnection_client_code.h"
    
    Buffer = ConnectToClient(&h2, client);
	
	if(Buffer == NULL)
	{
		DisconnectFromGlobal(h1, global);
		mexErrMsgIdAndTxt("MEX:GetRoiState:UnableToConnectClient", "Unable to connect to shared memory buffer (client state)! \n");
	}
		
		
	//////////////////////////////////////////////////////////////////////////
	// 3. Prepare the buffer for use
	//////////////////////////////////////////////////////////////////////////
	
	if(Prepare(global, client, EDS_GetRoiState) == FALSE)
	{
		DisconnectFromClient(h2, client);
		DisconnectFromGlobal(h1, global);
		mexErrMsgIdAndTxt("MEX:GetRoiState:ClientBlocked", "Server is still blocking the client channel! \n");
	}


    //////////////////////////////////////////////////////////////////////////
	// 5. Populate
	//////////////////////////////////////////////////////////////////////////

    bufferSize = 0;

    
	//////////////////////////////////////////////////////////////////////////
	// 5. Execute
	//////////////////////////////////////////////////////////////////////////
	
    if(Execute(global, client, (int)bufferSize) == FALSE)
    {
        DisconnectFromClient(h2, client);
        DisconnectFromGlobal(h1, global);
        mexErrMsgIdAndTxt("MEX:GetRoiState:ServerTimedOut", "SERVER TIMED OUT! \n");
    }


	//////////////////////////////////////////////////////////////////////////
	// 6. Echo the result
	//////////////////////////////////////////////////////////////////////////

	if(strncmp(client->Result, "success", 7) == 0)
	{
        resp = (DS_RoiState *)Buffer;
		if (nlhs == 0)
            mexPrintf("Number of ROIs = %d. Number of updates = %d.\n", resp->Count, resp->Frame);
    }
    else
		mexPrintf("Details (mxGetRoiState): %s\n", client->Details);
    
    
    //////////////////////////////////////////////////////////////////////////
	// 7. Populate Matlab output
	//////////////////////////////////////////////////////////////////////////
    
    if (nlhs == 1)
    {
        if (strncmp(client->Result, "success", 7) == 0)
        {    
            StateFieldNames_ptr[0] = "Count";
            StateFieldNames_ptr[1] = "LastUpdateTime";
            StateFieldNames_ptr[2] = "UpdateCount";
            dims[0] = 1;
            dims[1] = 1;
            StateStruct_ptr = mxCreateStructArray(2, dims, 3, (const char **)StateFieldNames_ptr);
            
            IntValue_ptr = (int *)mxCalloc(1, (mwSize)sizeof(int));
            *IntValue_ptr = resp->Count;
            dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
            dims[1] = 1;
            Data_ptr = mxCreateNumericArray(2, dims, mxINT32_CLASS, mxREAL);
            mxSetM(Data_ptr, 1);
            mxSetData(Data_ptr, IntValue_ptr);
            mxSetField(StateStruct_ptr, 0, (const char *)StateFieldNames_ptr[0], Data_ptr);
            
            Int64Value_ptr = (int64 *)mxCalloc(1, (mwSize)sizeof(int64));
            *Int64Value_ptr = resp->LastUpdateTime;
            dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
            dims[1] = 1;
            Data_ptr = mxCreateNumericArray(2, dims, mxINT64_CLASS, mxREAL);
            mxSetM(Data_ptr, 1);
            mxSetData(Data_ptr, Int64Value_ptr);
            mxSetField(StateStruct_ptr, 0, (const char *)StateFieldNames_ptr[1], Data_ptr);
            
            IntValue_ptr = (int *)mxCalloc(1, (mwSize)sizeof(int));
            *IntValue_ptr = resp->Frame;
            dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
            dims[1] = 1;
            Data_ptr = mxCreateNumericArray(2, dims, mxINT32_CLASS, mxREAL);
            mxSetM(Data_ptr, 1);
            mxSetData(Data_ptr, IntValue_ptr);
            mxSetField(StateStruct_ptr, 0, (const char *)StateFieldNames_ptr[2], Data_ptr);
            
            plhs[0] = StateStruct_ptr;
        }    
        else 
        {
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
