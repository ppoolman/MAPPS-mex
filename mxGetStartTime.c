/*=================================================================
 * mxGetStartTime.c 
 *
 *=================================================================*/

#include "mxDataIO.h"

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
#include "_SetUpConnection_global_declaration.h"
#include "_SetUpConnection_client_declaration.h"     

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
	// 2. Connect to the MAPPS shared memory buffer (Connection to client not made)
	//////////////////////////////////////////////////////////////////////////    
    
#include "_SetUpConnection_global_code.h"
#include "_SetUpConnection_client_code.h" 


	//////////////////////////////////////////////////////////////////////////
	// 3. Echo the result
	//////////////////////////////////////////////////////////////////////////
    
	if (nlhs == 0)
        mexPrintf("The project start time (FILETIME) is %lld. \n", global->Epoch);


    //////////////////////////////////////////////////////////////////////////
	// 4. Populate Matlab output
	//////////////////////////////////////////////////////////////////////////

    if (nlhs == 1) {
        dims[0] = 1;
        dims[1] = 1;
        plhs[0] = mxCreateNumericArray(2, dims, mxINT64_CLASS, mxREAL);
        memcpy((void *)mxGetData(plhs[0]), (const void *)&(global->Epoch), mxGetElementSize(plhs[0]));
    }

	
	//////////////////////////////////////////////////////////////////////////
	// 5. Disconnect from the MAPPS shared memory buffer
	//////////////////////////////////////////////////////////////////////////
	
	DisconnectFromGlobal(h1, global); 
}
