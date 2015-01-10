/*=================================================================
 * mxUpdateOptoDot.c 
 *
 *=================================================================*/

#include "mxDataIO.h"

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
#include "_SetUpConnection_global_declaration.h"
#include "_SetUpConnection_client_declaration.h"
    
    size_t bufferSize;
    int got_rx, got_ry, got_cr, got_cg, got_cb, got_ca;
    float x, y, rx, ry, cr, cg, cb, ca;
    char *Command_string, Value_string[256];
    void *Buffer;
    const mxArray *DotStruct_ptr;
    mxArray *Field_ptr;
     
    
    //////////////////////////////////////////////////////////////////////////
	// 0. Default values
	//////////////////////////////////////////////////////////////////////////
    
    got_rx = 0;
    got_ry = 0;
    got_cr = 0;
    got_cg = 0;
    got_cb = 0;
    got_ca = 0;
    

    //////////////////////////////////////////////////////////////////////////
	// 1. Check for proper number and type of input and output arguments
	////////////////////////////////////////////////////////////////////////// 
         
    if (!(nrhs == 2))
        mexErrMsgTxt("Input argument combination allowed is [struct, struct].\n");
    
    if (!(!(mxIsEmpty(prhs[0])) && mxIsStruct(prhs[0])))
        mexErrMsgTxt("First input argument is not a struct.\n");
    
    if (!(!(mxIsEmpty(prhs[1])) && mxIsStruct(prhs[1])))
        mexErrMsgTxt("Second input argument is not a struct.\n");
    
    
    //////////////////////////////////////////////////////////////////////////
	// 2. Extract data passed from Matlab
	////////////////////////////////////////////////////////////////////////// 
    
    DotStruct_ptr = prhs[1];
    
    Field_ptr = mxGetField(DotStruct_ptr, 0, "x");
    if (Field_ptr == NULL || mxIsEmpty(Field_ptr) || mxIsStruct(Field_ptr) || mxIsCell(Field_ptr) || !(mxIsNumeric(Field_ptr)))
        mexErrMsgTxt("No numeric field 'x' found.\n");
    else
        x = (float)mxGetScalar(Field_ptr);
    
    Field_ptr = mxGetField(DotStruct_ptr, 0, "y");
    if (Field_ptr == NULL || mxIsEmpty(Field_ptr) || mxIsStruct(Field_ptr) || mxIsCell(Field_ptr) || !(mxIsNumeric(Field_ptr)))
        mexErrMsgTxt("No numeric field 'y' found.\n");
    else
        y = (float)mxGetScalar(Field_ptr);
    
    if (mxGetNumberOfFields(DotStruct_ptr) > 2) {
        Field_ptr = mxGetField(DotStruct_ptr, 0, "rx");
        if (Field_ptr != NULL && !(mxIsEmpty(Field_ptr)) && !(mxIsStruct(Field_ptr)) && !(mxIsCell(Field_ptr)) && mxIsNumeric(Field_ptr)) {
            rx = (float)mxGetScalar(Field_ptr);
            got_rx = 1;
        }

        Field_ptr = mxGetField(DotStruct_ptr, 0, "ry");
        if (Field_ptr != NULL && !(mxIsEmpty(Field_ptr)) && !(mxIsStruct(Field_ptr)) && !(mxIsCell(Field_ptr)) && mxIsNumeric(Field_ptr)) {
            ry = (float)mxGetScalar(Field_ptr);
            got_ry = 1;
        }

        Field_ptr = mxGetField(DotStruct_ptr, 0, "cr");
        if (Field_ptr != NULL && !(mxIsEmpty(Field_ptr)) && !(mxIsStruct(Field_ptr)) && !(mxIsCell(Field_ptr)) && mxIsNumeric(Field_ptr)) {
            cr = (float)mxGetScalar(Field_ptr);
            got_cr = 1;
        }

        Field_ptr = mxGetField(DotStruct_ptr, 0, "cg");
        if (Field_ptr != NULL && !(mxIsEmpty(Field_ptr)) && !(mxIsStruct(Field_ptr)) && !(mxIsCell(Field_ptr)) && mxIsNumeric(Field_ptr)) {
            cg = (float)mxGetScalar(Field_ptr);
            got_cg = 1;
        }

        Field_ptr = mxGetField(DotStruct_ptr, 0, "cb");
        if (Field_ptr != NULL && !(mxIsEmpty(Field_ptr)) && !(mxIsStruct(Field_ptr)) && !(mxIsCell(Field_ptr)) && mxIsNumeric(Field_ptr)) {
            cb = (float)mxGetScalar(Field_ptr);
            got_cb = 1;
        }

        Field_ptr = mxGetField(DotStruct_ptr, 0, "ca");
        if (Field_ptr != NULL && !(mxIsEmpty(Field_ptr)) && !(mxIsStruct(Field_ptr)) && !(mxIsCell(Field_ptr)) && mxIsNumeric(Field_ptr)) {
            ca = (float)mxGetScalar(Field_ptr);
            got_ca = 1;
        }
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
		mexErrMsgIdAndTxt("MEX:UpdateOptoDot:UnableToConnectClient", "Unable to connect to shared memory buffer (client state)! \n");
	}
		
		
	//////////////////////////////////////////////////////////////////////////
	// 4. Prepare the buffer for use
	//////////////////////////////////////////////////////////////////////////
	
	if(Prepare(global, client, EDS_OptoClientMessage) == FALSE)
	{
		DisconnectFromClient(h2, client);
        DisconnectFromGlobal(h1, global);
		mexErrMsgIdAndTxt("MEX:UpdateOptoDot:ClientBlocked", "Server is still blocking the client channel! \n");
	}
	
    
	//////////////////////////////////////////////////////////////////////////
	// 5. Populate
	//////////////////////////////////////////////////////////////////////////
	
    Command_string = (char *)Buffer;
    sprintf(Command_string, "set_target:%f,%f", x, y);
    if (got_rx > 0) {
        sprintf(Value_string, ",%f", rx);
        strcat(Command_string, Value_string);
        if (got_ry > 0) {
            sprintf(Value_string, ",%f", ry);
            strcat(Command_string, Value_string);
            if (got_cr > 0 && got_cg > 0 && got_cb > 0 && got_ca > 0) {
                sprintf(Value_string, ",%f,%f,%f,%f", cr, cg, cb, ca);
                strcat(Command_string, Value_string);
            }
            else if (got_cr > 0 || got_cg > 0 || got_cb > 0 || got_ca > 0)
                mexPrintf("Warning (mxUpdateOptoDot): Incomplete color values -- all color values ignored.\n");
        }
    }
    else if (got_ry > 0 || got_cr > 0 || got_cg > 0 || got_cb > 0 || got_ca > 0)
        mexPrintf("Warning (mxUpdateOptoDot): Incomplete radius and/or color values -- all radius and color values ignored.\n");       
//    mexPrintf("Command string = %s\n", Command_string);
    
    bufferSize = strlen(Command_string) + 1;
    

	//////////////////////////////////////////////////////////////////////////
	// 6. Execute
	//////////////////////////////////////////////////////////////////////////
	
    if(Execute(global, client, (int)bufferSize) == FALSE)
    {
        DisconnectFromClient(h2, client);
        DisconnectFromGlobal(h1, global);
        mexErrMsgIdAndTxt("MEX:UpdateOptoDot:ServerTimedOut", "SERVER TIMED OUT! \n");
    }
    
    
	//////////////////////////////////////////////////////////////////////////
	// 7. Echo the result
	//////////////////////////////////////////////////////////////////////////
    
    if(strncmp(client->Result, "success", 7) != 0)
	    mexPrintf("Details (mxUpdateOptoDot): %s\n", client->Details); 
    
    
	//////////////////////////////////////////////////////////////////////////
	// 8. Close shared memory handles
	//////////////////////////////////////////////////////////////////////////

    DisconnectFromClient(h2, client);
    DisconnectFromGlobal(h1, global);
}
