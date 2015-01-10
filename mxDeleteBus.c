/*=================================================================
 * mxDeleteBus.c 
 *
 *=================================================================*/

#include "mxDataIO.h"

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
#include "_SetUpConnection_global_declaration.h"
#include "_SetUpConnection_client_declaration.h"
    
    size_t bufferSize;
    void *Buffer;
    DS_DeleteBusRequest *req;

    size_t name_length;
    char *Bus_string;
    

 	//////////////////////////////////////////////////////////////////////////
	// 1. Check for proper number and type of input and output arguments
	//////////////////////////////////////////////////////////////////////////     
    
    if (!(nrhs == 2))
        mexErrMsgTxt("Input argument combination allowed is [struct, string].\n");
    
    if (!(!(mxIsEmpty(prhs[0])) && mxIsStruct(prhs[0])))
        mexErrMsgTxt("First input argument is not a struct.\n");
    
    if (!(!(mxIsEmpty(prhs[1])) && mxIsChar(prhs[1]) && mxGetNumberOfDimensions(prhs[1]) == 2 && (mxGetM(prhs[1]) == 1 || mxGetN(prhs[1]) == 1)))
        mexErrMsgTxt("Second input argument is not a string.\n");
    
 
    //////////////////////////////////////////////////////////////////////////
	// 2. Extract data from input arguments
	//////////////////////////////////////////////////////////////////////////     

    name_length = mxGetNumberOfElements(prhs[1]) + 1;
    Bus_string = (char *)mxCalloc((mwSize)name_length, sizeof(char));
    if (mxGetString(prhs[1], Bus_string, (mwSize)name_length) != 0) {
        mexPrintf("\n\nCould not convert string data for second input argument.");
        mexErrMsgTxt("Conversion error.");
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
		mexErrMsgIdAndTxt("MEX:DeleteBus:UnableToConnectClient", "Unable to connect to shared memory buffer (client state)! \n");
	}
		
		
	//////////////////////////////////////////////////////////////////////////
	// 4. Prepare the buffer for use
	//////////////////////////////////////////////////////////////////////////
	
	if(Prepare(global, client, EDS_DeleteBus) == FALSE)
	{
		DisconnectFromClient(h2, client);
        DisconnectFromGlobal(h1, global);
		mexErrMsgIdAndTxt("MEX:DeleteBus:ClientBlocked", "Server is still blocking the client channel! \n");
	}
	
    
	//////////////////////////////////////////////////////////////////////////
	// 5. Populate
	//////////////////////////////////////////////////////////////////////////
	
    req = (DS_DeleteBusRequest *)Buffer;
    bufferSize = 0;
    
	sprintf(req->BusName, Bus_string);
    
    bufferSize += sizeof(DS_DeleteBusRequest);
	
	
	//////////////////////////////////////////////////////////////////////////
	// 6. Execute
	//////////////////////////////////////////////////////////////////////////
	
    if(Execute(global, client, (int)bufferSize) == FALSE)
    {
        DisconnectFromClient(h2, client);
        DisconnectFromGlobal(h1, global);
        mexErrMsgIdAndTxt("MEX:DeleteBus:ServerTimedOut", "SERVER TIMED OUT! \n");
    }
    
    
	//////////////////////////////////////////////////////////////////////////
	// 7. Echo the result
	//////////////////////////////////////////////////////////////////////////
    
    if(strncmp(client->Result, "success", 7) != 0)
	    mexPrintf("Details (mxDeleteBus): %s\n", client->Details);
    
    
	//////////////////////////////////////////////////////////////////////////
	// 8. Close shared memory handles
	//////////////////////////////////////////////////////////////////////////

    DisconnectFromClient(h2, client);
    DisconnectFromGlobal(h1, global);
    
    
    //////////////////////////////////////////////////////////////////////////
	// 9. Free allocated memory
	//////////////////////////////////////////////////////////////////////////
    
    mxFree(Bus_string);
}
