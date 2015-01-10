/*=================================================================
 * mxGetMessages.c 
 *
 *=================================================================*/

#include "mxDataIO.h"

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
#include "_SetUpConnection_global_declaration.h"
#include "_SetUpConnection_client_declaration.h"
    
    size_t bufferSize;
    void *Buffer;   
    
    int i;
    mwSize dims[2];
    char Fieldname_string[32], Number_string[4];
    char *Fieldnames[1];
    mxArray *MessageStruct_ptr, *Message_ptr;
    
    
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
		mexErrMsgIdAndTxt("MEX:GetMessages:UnableToConnectClient", "Unable to connect to shared memory buffer (client state)! \n");
	}


    //////////////////////////////////////////////////////////////////////////
	// 3. Check message count
	//////////////////////////////////////////////////////////////////////////
	
    if(client->MessageCount == 0)
    {
        dims[0] = 0;
        dims[1] = 1;
        plhs[0] = mxCreateNumericArray(2, dims, mxCHAR_CLASS, mxREAL);
        return;
    }
		
	//////////////////////////////////////////////////////////////////////////
	// 4. Prepare the buffer for use
	//////////////////////////////////////////////////////////////////////////
	
	if(Prepare(global, client, EDS_GetMessage) == FALSE)
	{
		DisconnectFromClient(h2, client);
		DisconnectFromGlobal(h1, global);
		mexErrMsgIdAndTxt("MEX:GetMessages:ClientBlocked", "Server is still blocking the client channel! \n");
	}

    
	//////////////////////////////////////////////////////////////////////////
	// 5. Execute and populate Matlab output
	//////////////////////////////////////////////////////////////////////////
	
    bufferSize = 0;

    i = 1;    
    strcpy(Fieldname_string, "message_"); 
    sprintf(Number_string, "%d", i);
    strcat(Fieldname_string, Number_string);
    
    dims[0] = 1;
    dims[1] = 1;
    Fieldnames[0] = Fieldname_string;
    MessageStruct_ptr = mxCreateStructArray(2, dims, 1, (const char **)Fieldnames);
    while (client->MessageCount != 0)
    {
        if(Execute(global, client, (int)bufferSize) == FALSE)
        {
            DisconnectFromClient(h2, client);
            DisconnectFromGlobal(h1, global);
            mexErrMsgIdAndTxt("MEX:GetMessages:ServerTimedOut", "SERVER TIMED OUT! \n");
        }

        if(strncmp(client->Result, "success", 7) == 0)
        {
            Message_ptr = mxCreateString((const char *)Buffer);
        }
        else
        {
            dims[0] = 0;
            dims[1] = 1;
            Message_ptr = mxCreateNumericArray(2, dims, mxCHAR_CLASS, mxREAL);
            mexPrintf("Details (mxGetMessages) for Message %d: %s\n", i, client->Details);
        }
        
        mxSetField(MessageStruct_ptr, 0, Fieldname_string, Message_ptr);
        
        if(client->MessageCount != 0)
        {
            i++;
            strcpy(Fieldname_string, "message_"); 
            sprintf(Number_string, "%d", i);
            strcat(Fieldname_string, Number_string);
            mxAddField(MessageStruct_ptr, Fieldname_string);
        }
    }
    
    plhs[0] = MessageStruct_ptr;
    
    //////////////////////////////////////////////////////////////////////////
	// 6. Close shared memory handles
	//////////////////////////////////////////////////////////////////////////

	DisconnectFromClient(h2, client);
	DisconnectFromGlobal(h1, global);
}
