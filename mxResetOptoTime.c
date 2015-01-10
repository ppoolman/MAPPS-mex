/*=================================================================
 * mxResetOptoTime.c 
 *
 *=================================================================*/

#include "mxDataIO.h"

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
#include "_SetUpConnection_global_declaration.h"
#include "_SetUpConnection_client_declaration.h"
    
    size_t bufferSize;
    char *Command_string;
    void *Buffer;
     

    //////////////////////////////////////////////////////////////////////////
	// 1. Check for proper number and type of input and output arguments
	////////////////////////////////////////////////////////////////////////// 
         
    if (!(nrhs == 1))
        mexErrMsgTxt("Input argument combination allowed is [struct].\n");
    
    if (!(!(mxIsEmpty(prhs[0])) && mxIsStruct(prhs[0])))
        mexErrMsgTxt("First input argument is not a struct.\n");
        
    
    //////////////////////////////////////////////////////////////////////////
	// 2. Connect to the MAPPS shared memory buffer
	//////////////////////////////////////////////////////////////////////////    
    
#include "_SetUpConnection_global_code.h"
#include "_SetUpConnection_client_code.h"
    
    Buffer = ConnectToClient(&h2, client);
	
	if(Buffer == NULL)
	{
		DisconnectFromGlobal(h1, global);
		mexErrMsgIdAndTxt("MEX:ResetOptoTime:UnableToConnectClient", "Unable to connect to shared memory buffer (client state)! \n");
	}
		
		
	//////////////////////////////////////////////////////////////////////////
	// 3. Prepare the buffer for use
	//////////////////////////////////////////////////////////////////////////
	
	if(Prepare(global, client, EDS_OptoSupMessage) == FALSE)
	{
		DisconnectFromClient(h2, client);
        DisconnectFromGlobal(h1, global);
		mexErrMsgIdAndTxt("MEX:ResetOptoTime:ClientBlocked", "Server is still blocking the client channel! \n");
	}
	
    
	//////////////////////////////////////////////////////////////////////////
	// 4. Populate
	//////////////////////////////////////////////////////////////////////////
	
    Command_string = (char *)Buffer;
    sprintf(Command_string, "sup_reset_time: ");
    bufferSize = strlen(Command_string) + 1;
    

	//////////////////////////////////////////////////////////////////////////
	// 5. Execute
	//////////////////////////////////////////////////////////////////////////
	
    if(Execute(global, client, (int)bufferSize) == FALSE)
    {
        DisconnectFromClient(h2, client);
        DisconnectFromGlobal(h1, global);
        mexErrMsgIdAndTxt("MEX:ResetOptoTime:ServerTimedOut", "SERVER TIMED OUT! \n");
    }
    
    
	//////////////////////////////////////////////////////////////////////////
	// 6. Echo the result
	//////////////////////////////////////////////////////////////////////////
    
    if(strncmp(client->Result, "success", 7) != 0)
	    mexPrintf("Details (mxResetOptoTime): %s\n", client->Details); 
    
    
	//////////////////////////////////////////////////////////////////////////
	// 7. Close shared memory handles
	//////////////////////////////////////////////////////////////////////////

    DisconnectFromClient(h2, client);
    DisconnectFromGlobal(h1, global);
}
