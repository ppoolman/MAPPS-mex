/*=================================================================
 * mxDeleteByTime.c 
 *
 *=================================================================*/

#include "mxDataIO.h"

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
#include "_SetUpConnection_global_declaration.h"
#include "_SetUpConnection_client_declaration.h"
    
    size_t bufferSize;
    void *Buffer;
    DS_DeleteFramesRequest *req;

    size_t name_length;
    int64 startTime, endTime, swappedTime;
    char *Bus_string;

    
	//////////////////////////////////////////////////////////////////////////
	// 1. Check for proper number and type of input and output arguments
	////////////////////////////////////////////////////////////////////////// 
    
    /* prhs[1] <-- name of bus
     * prhs[2] <-- inclusive start time
     * prhs[3] <-- inclusive end time */
    
    /* Set 'DeleteByIndex' to 1 if you want to delete by index (StartIndex and EndIndex). 
     * If zero, then it will use the time fields (StartTime and EndTime). 
     * Both are range inclusive.
     * Errors are a first frame earlier than the beginning, an end frame past the end or a start frame greater than the last frame. */
    
    if (!(nrhs == 4))
        mexErrMsgTxt("Input argument combination allowed is [struct, string, scalar, scalar].\n");
    
    if (!(!(mxIsEmpty(prhs[0])) && mxIsStruct(prhs[0])))
        mexErrMsgTxt("First input argument is not a struct.\n");
    
    if (!(!(mxIsEmpty(prhs[1])) && mxIsChar(prhs[1]) && mxGetNumberOfDimensions(prhs[1]) == 2 && (mxGetM(prhs[1]) == 1 || mxGetN(prhs[1]) == 1)))
        mexErrMsgTxt("Second input argument is not a string.\n");
    
    if (!(!(mxIsEmpty(prhs[2])) && mxIsNumeric(prhs[2]) && mxGetNumberOfDimensions(prhs[2]) == 2 && mxGetM(prhs[2]) == 1 && mxGetN(prhs[2]) == 1))
        mexErrMsgTxt("Third input argument is not a scalar.\n");
    
    if (!(!(mxIsEmpty(prhs[3])) && mxIsNumeric(prhs[3]) && mxGetNumberOfDimensions(prhs[3]) == 2 && mxGetM(prhs[3]) == 1 && mxGetN(prhs[3]) == 1))
        mexErrMsgTxt("Fourth input argument is not a scalar.\n");
    
    
    //////////////////////////////////////////////////////////////////////////
	// 2. Extract data passed from Matlab
	////////////////////////////////////////////////////////////////////////// 
    
    name_length = mxGetNumberOfElements(prhs[1]) + 1;
    Bus_string = (char *)mxCalloc((mwSize)name_length, sizeof(char));
    if (mxGetString(prhs[1], Bus_string, (mwSize)name_length) != 0) {
        mexPrintf("\n\nCould not convert string data for second input argument.");
        mexErrMsgTxt("Conversion error.");
    }
    
    startTime = (int64)mxGetScalar(prhs[2]);
    endTime = (int64)mxGetScalar(prhs[3]);
    if (startTime > endTime) {
        swappedTime = startTime;
        startTime = endTime;
        endTime = swappedTime;
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
		mexErrMsgIdAndTxt("MEX:DeleteByTime:UnableToConnectClient", "Unable to connect to shared memory buffer (client state)! \n");
	}
		
		
	//////////////////////////////////////////////////////////////////////////
	// 4. Prepare the buffer for use
	//////////////////////////////////////////////////////////////////////////
	
	if(Prepare(global, client, EDS_DeleteFrames) == FALSE)
	{
		DisconnectFromClient(h2, client);
        DisconnectFromGlobal(h1, global);
		mexErrMsgIdAndTxt("MEX:DeleteByTime:ClientBlocked", "Server is still blocking the client channel! \n");
	}
	
    
	//////////////////////////////////////////////////////////////////////////
	// 5. Populate
	//////////////////////////////////////////////////////////////////////////
	
    req = (DS_DeleteFramesRequest *)Buffer;
    bufferSize = 0;
    
	sprintf(req->BusName, Bus_string);
    req->DeleteByIndex = 0;
	req->StartTime = startTime;
    req->EndTime = endTime;
    
    bufferSize += sizeof(DS_DeleteFramesRequest);
	
	
	//////////////////////////////////////////////////////////////////////////
	// 6. Execute
	//////////////////////////////////////////////////////////////////////////
	
    if(Execute(global, client, (int)bufferSize) == FALSE)
    {
        DisconnectFromClient(h2, client);
        DisconnectFromGlobal(h1, global);
        mexErrMsgIdAndTxt("MEX:DeleteByTime:ServerTimedOut", "SERVER TIMED OUT! \n");
    }
    
    
	//////////////////////////////////////////////////////////////////////////
	// 7. Echo the result
	//////////////////////////////////////////////////////////////////////////
    
    if(strncmp(client->Result, "success", 7) != 0)
	    mexPrintf("Details (mxDeleteByTime): %s\n", client->Details);
    
    
	//////////////////////////////////////////////////////////////////////////
	// 8. Close shared memory handles
	//////////////////////////////////////////////////////////////////////////

    DisconnectFromClient(h2, client);
    DisconnectFromGlobal(h1, global);

	
	//////////////////////////////////////////////////////////////////////////
	// 9. Clean up allocated memory
	//////////////////////////////////////////////////////////////////////////

    mxFree(Bus_string);
}
