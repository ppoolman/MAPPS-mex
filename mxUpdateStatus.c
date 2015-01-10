/*=================================================================
 * mxUpdateStatus.c 
 *
 *=================================================================*/

#include "mxDataIO.h"

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
#include "_SetUpConnection_global_declaration.h"
#include "_SetUpConnection_client_declaration.h"
       
    size_t name_length;
    

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
	// 2. Connect to the MAPPS shared memory buffer
	//////////////////////////////////////////////////////////////////////////    
    
#include "_SetUpConnection_global_code.h"
#include "_SetUpConnection_client_code.h"
    
		
    //////////////////////////////////////////////////////////////////////////
	// 3. Populate
	//////////////////////////////////////////////////////////////////////////

    name_length = mxGetNumberOfElements(prhs[1]) + 1;
    if (name_length > sizeof(client->Status))
        name_length = sizeof(client->Status);
    mxGetString(prhs[1], client->Status, (mwSize)name_length);
  
    
	//////////////////////////////////////////////////////////////////////////
	// 4. Close shared memory handles
	//////////////////////////////////////////////////////////////////////////

    DisconnectFromGlobal(h1, global);
}
