/*=================================================================
 * mxLoadOptoProject.c 
 *
 *=================================================================*/

#include "mxDataIO.h"

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
#include "_SetUpConnection_global_declaration.h"
#include "_SetUpConnection_client_declaration.h"
    
    size_t name_length, bufferSize;
    char *Command_string, *Filename_string;
    void *Buffer;
     

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
    Filename_string = (char *)mxCalloc((mwSize)name_length, sizeof(char));
    if (mxGetString(prhs[1], Filename_string, (mwSize)name_length) != 0) {
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
		mexErrMsgIdAndTxt("MEX:LoadOptoProject:UnableToConnectClient", "Unable to connect to shared memory buffer (client state)! \n");
	}
		
		
	//////////////////////////////////////////////////////////////////////////
	// 4. Prepare the buffer for use
	//////////////////////////////////////////////////////////////////////////
	
	if(Prepare(global, client, EDS_OptoSupMessage) == FALSE)
	{
		DisconnectFromClient(h2, client);
        DisconnectFromGlobal(h1, global);
		mexErrMsgIdAndTxt("MEX:LoadOptoProject:ClientBlocked", "Server is still blocking the client channel! \n");
	}
	
    
	//////////////////////////////////////////////////////////////////////////
	// 5. Populate
	//////////////////////////////////////////////////////////////////////////
	
    Command_string = (char *)Buffer;
    sprintf(Command_string, "sup_load_project:%s", Filename_string);
    bufferSize = strlen(Command_string) + 1;
    

	//////////////////////////////////////////////////////////////////////////
	// 6. Execute
	//////////////////////////////////////////////////////////////////////////
	
    if(Execute(global, client, (int)bufferSize) == FALSE)
    {
        DisconnectFromClient(h2, client);
        DisconnectFromGlobal(h1, global);
        mexErrMsgIdAndTxt("MEX:LoadOptoProject:ServerTimedOut", "SERVER TIMED OUT! \n");
    }
    
    
	//////////////////////////////////////////////////////////////////////////
	// 7. Echo the result
	//////////////////////////////////////////////////////////////////////////
    
    if(strncmp(client->Result, "success", 7) != 0)
	    mexPrintf("Details (mxLoadOptoProject): %s\n", client->Details); 
    
    
	//////////////////////////////////////////////////////////////////////////
	// 8. Close shared memory handles
	//////////////////////////////////////////////////////////////////////////

    DisconnectFromClient(h2, client);
    DisconnectFromGlobal(h1, global);
    
    
    //////////////////////////////////////////////////////////////////////////
	// 9. Free allocated memory
	//////////////////////////////////////////////////////////////////////////
    
    mxFree(Filename_string);
}
