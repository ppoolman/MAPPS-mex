/*=================================================================
 * mxListCanvases.c 
 *
 *=================================================================*/

#include "mxDataIO.h"

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
#include "_SetUpConnection_global_declaration.h"
#include "_SetUpConnection_client_declaration.h"
    
    size_t bufferSize;
    void *Buffer; 
    DS_CanvasSizesResponse *resp;
    
	char **CanvasNames_ptr, *Sizes_ptr[2]; 
    double *SizeValue_ptr;
    mxArray *CanvasesStruct_ptr, *Data_ptr;
    mwSize dims[2];
    int i, j;
    
    
    //////////////////////////////////////////////////////////////////////////
	// 1. Check for proper number of input and output arguments
	//////////////////////////////////////////////////////////////////////////
    
    if (!(nrhs == 1))
        mexErrMsgTxt("Input argument combination allowed is [struct].\n");
    
    if (!(!(mxIsEmpty(prhs[0])) && mxIsStruct(prhs[0])))
        mexErrMsgTxt("First input argument is not a struct.\n");
    
    if (nlhs > 0 && nlhs != 1){
        mexErrMsgTxt("A single output argument is required.\n");
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
		mexErrMsgIdAndTxt("MEX:ListCanvases:UnableToConnectClient", "Unable to connect to shared memory buffer (client state)! \n");
	}
		
		
	//////////////////////////////////////////////////////////////////////////
	// 3. Prepare the buffer for use
	//////////////////////////////////////////////////////////////////////////
	
	if(Prepare(global, client, EDS_GetCanvasSizes) == FALSE)
	{
		DisconnectFromClient(h2, client);
        DisconnectFromGlobal(h1, global);
		mexErrMsgIdAndTxt("MEX:ListCanvases:ClientBlocked", "Server is still blocking the client channel! \n");
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
        mexErrMsgIdAndTxt("MEX:ListCanvases:ServerTimedOut", "SERVER TIMED OUT! \n");
    }


	//////////////////////////////////////////////////////////////////////////
	// 6. Echo the result
	//////////////////////////////////////////////////////////////////////////

	if(strncmp(client->Result, "success", 7) == 0)
    {
        resp = (DS_CanvasSizesResponse *)Buffer;
        if (nlhs == 0) {
            mexPrintf("There %s %d %s \n", 
                      resp->CanvasCount == 1 ? "is" : "are",
                      resp->CanvasCount, 
                      resp->CanvasCount == 1 ? "canvas:" : "canvases:");

            for(i=0; i<resp->CanvasCount; i++)
            {
                mexPrintf("  [%d - '%s']:  %d x %d pixels \n", 
                          i+1, 
                          resp->Canvases[i].Name, 
                          resp->Canvases[i].Width, 
                          resp->Canvases[i].Height);
            }
        }
	}
	else
	{
		mexPrintf("Details (mxListCanvases): %s\n", client->Details);
	}
    
    
    //////////////////////////////////////////////////////////////////////////
	// 7. Populate Matlab output
	//////////////////////////////////////////////////////////////////////////
    
    if (nlhs == 1) {
        if (strncmp(client->Result, "success", 7) == 0) {
            CanvasNames_ptr = (char **)mxCalloc((mwSize)resp->CanvasCount, (mwSize)sizeof(char *));
            for (i = 0; i < resp->CanvasCount; i++) {
                CanvasNames_ptr[i] = resp->Canvases[i].Name;
            }
            dims[0] = 1;
            dims[1] = 1;
            CanvasesStruct_ptr = mxCreateStructArray(2, dims, (int)resp->CanvasCount, (const char **)CanvasNames_ptr);

            Sizes_ptr[0] = "Width";
            Sizes_ptr[1] = "Height";
            for (i = 0; i < resp->CanvasCount; i++) {
                dims[0] = 1;
                dims[1] = 1;
                mxSetField(CanvasesStruct_ptr, 0, (const char *)CanvasNames_ptr[i], mxCreateStructArray(2, dims, 2, (const char **)Sizes_ptr));

                for (j = 0; j < 2; j++) {
                    SizeValue_ptr = (double *)mxCalloc(1, (mwSize)sizeof(double));
                    if (j == 0) {
                        *SizeValue_ptr = (double)resp->Canvases[i].Width;
                    }
                    else {
                        *SizeValue_ptr = (double)resp->Canvases[i].Height;
                    }
                    dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
                    dims[1] = 1;
                    Data_ptr = mxCreateNumericArray(2, dims, mxDOUBLE_CLASS, mxREAL);
                    mxSetM(Data_ptr, 1);
                    mxSetData(Data_ptr, SizeValue_ptr); 
                    mxSetField(mxGetField(CanvasesStruct_ptr, 0, (const char *)CanvasNames_ptr[i]), 0, Sizes_ptr[j], Data_ptr);
                }
            }

            plhs[0] = CanvasesStruct_ptr;

            mxFree(CanvasNames_ptr);
        }
        else {
            dims[0] = 0;
            dims[1] = 1;
            plhs[0] = mxCreateNumericArray(2, dims, mxDOUBLE_CLASS, mxREAL);
        }
    }

	
	//////////////////////////////////////////////////////////////////////////
	// 8. Close shared memory handles
	//////////////////////////////////////////////////////////////////////////

    DisconnectFromClient(h2, client);
    DisconnectFromGlobal(h1, global);
}
