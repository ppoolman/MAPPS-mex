/*=================================================================
 * mxComputeFixations.c 
 *
 *=================================================================*/

#include "mxDataIO.h"

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
#include "_SetUpConnection_global_declaration.h"
#include "_SetUpConnection_client_declaration.h"
    
    size_t bufferSize;
    void *Buffer;

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
		mexErrMsgIdAndTxt("MEX:ComputeFixations:UnableToConnectClient", "Unable to connect to shared memory buffer (client state)! \n");
	}
		
		
	//////////////////////////////////////////////////////////////////////////
	// 3. Prepare the buffer for use
	//////////////////////////////////////////////////////////////////////////
	
	if(Prepare(global, client, EDS_UpdateFixations) == FALSE)
	{
		DisconnectFromClient(h2, client);
        DisconnectFromGlobal(h1, global);
		mexErrMsgIdAndTxt("MEX:ComputeFixations:ClientBlocked", "Server is still blocking the client channel! \n");
	}


    //////////////////////////////////////////////////////////////////////////
	// 4. Populate
	//////////////////////////////////////////////////////////////////////////
	
    bufferSize = 0;

    	
	//////////////////////////////////////////////////////////////////////////
	// 5. Execute
	//////////////////////////////////////////////////////////////////////////
	
	if(Execute(global, client, (int)bufferSize) == FALSE)
    {
        DisconnectFromClient(h2, client);
        DisconnectFromGlobal(h1, global);
        mexErrMsgIdAndTxt("MEX:ComputeFixations:ServerTimedOut", "SERVER TIMED OUT! \n");
    }
	

    //////////////////////////////////////////////////////////////////////////
	// 6. Echo the result
	//////////////////////////////////////////////////////////////////////////

	if(strncmp(client->Result, "success", 7) != 0 && nlhs == 0)
        mexPrintf("Details (mxComputeFixations): %s\n", client->Details);
	
    
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
