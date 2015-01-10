/*=================================================================
 * mxSetTime.c 
 *
 *=================================================================*/

#include "mxDataIO.h"

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
#include "_SetUpConnection_global_declaration.h"
#include "_SetUpConnection_client_declaration.h" 
    
    size_t bufferSize;
    void *Buffer;
    DS_SetTimeRequest *timeReq;
    
    mwSize dims[2];
    
    
    //////////////////////////////////////////////////////////////////////////
	// 1. Check for proper number of input and output arguments
	//////////////////////////////////////////////////////////////////////////
    
    if (!(nrhs == 2 || nrhs == 3))
        mexErrMsgTxt("Input argument combinations allowed are [struct, scalar] or [struct, scalar, character].\n");
    
    if (!(!(mxIsEmpty(prhs[0])) && mxIsStruct(prhs[0])))
        mexErrMsgTxt("First input argument is not a struct.\n");
    
    if (!(!(mxIsEmpty(prhs[1])) && mxIsNumeric(prhs[1]) && mxGetNumberOfDimensions(prhs[1]) == 2 && mxGetM(prhs[1]) == 1 && mxGetN(prhs[1]) == 1))
        mexErrMsgTxt("Second input argument is not a scalar.\n");
    
    if (nrhs == 3 && (!(!(mxIsEmpty(prhs[2])) && mxIsChar(prhs[2]) && mxGetNumberOfDimensions(prhs[2]) == 2 && mxGetM(prhs[2]) == 1 && mxGetN(prhs[2]) == 1)))
        mexErrMsgTxt("Third input argument is not a character.\n");
    
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
		mexErrMsgIdAndTxt("MEX:SetTime:UnableToConnectClient", "Unable to connect to shared memory buffer (client state)! \n");
	}
		
		
	//////////////////////////////////////////////////////////////////////////
	// 3. Prepare the buffer for use
	//////////////////////////////////////////////////////////////////////////
	
	if(Prepare(global, client, EDS_SetTime) == FALSE)
	{
		DisconnectFromClient(h2, client);
        DisconnectFromGlobal(h1, global);
		mexErrMsgIdAndTxt("MEX:SetTime:ClientBlocked", "Server is still blocking the client channel! \n");
	}
	
    
	//////////////////////////////////////////////////////////////////////////
	// 4. Populate
	//////////////////////////////////////////////////////////////////////////
	
    timeReq = (DS_SetTimeRequest *)Buffer;
    bufferSize = 0;
    
	timeReq->Time = (float)mxGetScalar(prhs[1]);
    if (nrhs == 3)
        mxGetString(prhs[2], &timeReq->Mode, 2);        //Set it to NULL or 't' to transition to; 's' to pause at; 'a' to play at
    else
        sprintf(&timeReq->Mode, "t");
    
    bufferSize += sizeof(DS_SetTimeRequest);
	
	
	//////////////////////////////////////////////////////////////////////////
	// 5. Execute
	//////////////////////////////////////////////////////////////////////////
	
    if(Execute(global, client, (int)bufferSize) == FALSE)
    {
        DisconnectFromClient(h2, client);
        DisconnectFromGlobal(h1, global);
        mexErrMsgIdAndTxt("MEX:SetTime:ServerTimedOut", "SERVER TIMED OUT! \n");
    }
	
	
    //////////////////////////////////////////////////////////////////////////
	// 6. Echo the result
	//////////////////////////////////////////////////////////////////////////

	if(strncmp(client->Result, "success", 7) != 0 && nlhs == 0)
        mexPrintf("Details (mxSetTime): %s\n", client->Details);
	
    
    //////////////////////////////////////////////////////////////////////////
	// 7. Populate Matlab output
	//////////////////////////////////////////////////////////////////////////

    if(nlhs == 1)
    {
        dims[0] = 0;
        dims[1] = 1;
        plhs[0] = mxCreateNumericArray(2, dims, mxDOUBLE_CLASS, mxREAL);
    }


	//////////////////////////////////////////////////////////////////////////
	// 8. Disconnect from the MAPPS shared memory buffer
	//////////////////////////////////////////////////////////////////////////
	
	DisconnectFromClient(h2, client);
    DisconnectFromGlobal(h1, global);
}
