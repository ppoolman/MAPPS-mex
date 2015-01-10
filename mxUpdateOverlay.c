/*=================================================================
 * mxUpdateOverlay.c 
 *
 *=================================================================*/

#include "mxDataIO.h"

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{ 
#include "_SetUpConnection_global_declaration.h"
#include "_SetUpConnection_client_declaration.h"
    
    size_t bufferSize;
    void *Buffer;
    DS_SetCanvas *req;

    const mwSize *Dims3D;
    mxArray *prhs_mCM[3];
    char *Name_string, *Order_string, *Mode;
    mxArray *ImageArray, *AlphaArray;
    unsigned char *KronMask, *Ptr_UINT8;
    unsigned int *FlipMask;
    mwSize dims[3];
    bool *flipImageUD;
    double *PermuteMask, *ConcatMask;
    void *MAPPS_ImageArray;
	int width, height;
    mwSize nDim;
    size_t name_length, nPixels, i, beforeKronM, beforeKronN;
    
    
    //////////////////////////////////////////////////////////////////////////
	// 0. Initialize variables
	////////////////////////////////////////////////////////////////////////// 
    
    Name_string = NULL;
    Order_string = NULL;
    ImageArray = NULL;
    
    
    //////////////////////////////////////////////////////////////////////////
	// 1. Check for proper number of input and output arguments
	//////////////////////////////////////////////////////////////////////////
    
    /*  prhs[1] <-- String containing canvas name
     *  prhs[2] <-- 3D/2D array containing image data
     *  prhs[3] <-- boolean value to specify whether image has to be flipped vertically
     *  prhs[4] <-- [Optional] String containing ordering of dimensions, e.g., Matlab RGB image is 'HWC' for [H]eight (1st dim), [W]idth (2nd dim), [C]olor info (3rd fim)
     *              The ordering expected by MAPPS is 'CWH' 
     
     *  plhs[0] <-- error code */
    
    if (!(nrhs == 4 || nrhs == 5))
        mexErrMsgTxt("Input argument combinations allowed are:\n * [struct, string, 3D/2D numeric array, boolean (flip up-down?)], assuming Matlab default ordering of [H]eight, [W]idth, [C]olor\n * [struct, string, 3D/2D numeric array, boolean (flip up-down?), string (3/2 letter combination specifying ordering of [H]eight, [W]idth, [C]olor)].");
    
    if (!(!(mxIsEmpty(prhs[0])) && mxIsStruct(prhs[0])))
        mexErrMsgTxt("First input argument is not a struct.\n");
    
    if (!(!(mxIsEmpty(prhs[1])) && mxIsChar(prhs[1]) && mxGetNumberOfDimensions(prhs[1]) == 2 && (mxGetM(prhs[1]) == 1 || mxGetN(prhs[1]) == 1)))
        mexErrMsgTxt("Second input argument is not a string.\n");
    
    if (!(!(mxIsEmpty(prhs[2])) && mxIsNumeric(prhs[2]) && (mxGetNumberOfDimensions(prhs[2]) == 2 || mxGetNumberOfDimensions(prhs[2]) == 3)))
        mexErrMsgTxt("Third input argument is not a 3D/2D numeric array.\n");
    
    if (!(!(mxIsEmpty(prhs[3])) && mxIsLogical(prhs[3]) && mxGetM(prhs[3]) == 1 && mxGetN(prhs[3]) == 1))
        mexErrMsgTxt("Fourth input argument is not a boolean.\n");
    
    if (nrhs == 5)
    {
        if (!(!(mxIsEmpty(prhs[4])) && mxIsChar(prhs[4]) && (mxGetM(prhs[4]) == 1 || mxGetN(prhs[4]) == 1) && mxGetNumberOfElements(prhs[4]) > 1 && mxGetNumberOfElements(prhs[4]) < 4))
            mexErrMsgTxt("Fifth input argument is not a string (3/2 letters).\n");
    }
          
    if (!(nlhs == 0 || nlhs == 1))
        mexErrMsgTxt("No more than a single output argument is required.\n");
    
    
    //////////////////////////////////////////////////////////////////////////
	// 2. Extract data passed from Matlab
	////////////////////////////////////////////////////////////////////////// 
    
    name_length = mxGetNumberOfElements(prhs[1]) + 1;
    Name_string = (char *)mxCalloc((mwSize)name_length, sizeof(char));
    if (mxGetString(prhs[1], Name_string, (mwSize)name_length) != 0) {
        mexPrintf("\n\nCould not convert string data for canvas name.");
        mexErrMsgTxt("Conversion error.");
    }
    
    nDim = mxGetNumberOfDimensions(prhs[2]);
    
    if (mxIsUint8(prhs[2]) != 1) {
        mexPrintf("Data transfer of image might be faster if passed as 'uint8' from Matlab. \n");
        mexCallMATLAB(1, &ImageArray, 1, (mxArray **)&prhs[2], "uint8");
    }
    else {
        ImageArray = (mxArray *)prhs[2];                                        // Caution
    }
    
    flipImageUD = (bool *)mxGetData(prhs[3]);
    if (*flipImageUD) {
        mexPrintf("Data transfer of image might be faster if passed up-down flipped from Matlab. \n");
    }
        
    if (nrhs == 4) {
        if (nDim == 2)
            name_length = 3;
        else if (nDim == 3) 
            name_length = 4;
        Order_string = (char *)mxCalloc((mwSize)name_length, sizeof(char));
        if (nDim == 2)
            strncpy(Order_string, "HW", name_length - 1);
        else if (nDim == 3)
            strncpy(Order_string, "HWC", name_length - 1);
    }
    else if (nrhs == 5) {
        name_length = mxGetNumberOfElements(prhs[4]) + 1;
        Order_string = (char *)mxCalloc((mwSize)name_length, sizeof(char));
        if (mxGetString(prhs[4], Order_string, (mwSize)name_length) != 0) {
            mexPrintf("\n\nCould not convert string data for ordering of dimensions.");
            mexErrMsgTxt("Conversion error.");
        }
    }
        
    if (nDim == 2) {
        
        Mode = "RGBA";
        
        if (strncmp(Order_string, "HW", 2) == 0 || strncmp(Order_string, "hw", 2) == 0) {          
            height = (int)mxGetM(ImageArray);
            width = (int)mxGetN(ImageArray);
            nPixels = (size_t)width * height;
            
            if (*flipImageUD) {
                prhs_mCM[0] = ImageArray;
                mexCallMATLAB(1, &ImageArray, 1, prhs_mCM, "flipud");
                if (prhs_mCM[0] != prhs[2]) {                                   // Caution
                    mxDestroyArray(prhs_mCM[0]);
                }
            }
            
            mexPrintf("Data transfer of image might be faster if passed transposed from Matlab. \n");
            prhs_mCM[0] = ImageArray;
            mexCallMATLAB(1, &ImageArray, 1, prhs_mCM, "transpose");
            if (prhs_mCM[0] != prhs[2]) {                                       // Caution
                mxDestroyArray(prhs_mCM[0]);
            }
        }
        else if (strncmp(Order_string, "WH", 2) == 0 || strncmp(Order_string, "wh", 2) == 0) {
            width = (int)mxGetM(ImageArray);
            height = (int)mxGetN(ImageArray);
            nPixels = (size_t)width * height;
            
            if (*flipImageUD) {
                prhs_mCM[0] = ImageArray;
                mexCallMATLAB(1, &ImageArray, 1, prhs_mCM, "fliplr");
                if (prhs_mCM[0] != prhs[2]) {                                   // Caution
                    mxDestroyArray(prhs_mCM[0]);
                }
            }
        }
        else {
            mexErrMsgTxt("Unrecognized/unsupported ordering.");
        }

        if (strcmp(Mode, "RGBA") == 0) {
            mexPrintf("Data transfer of gray scale image might be faster if passed as RGBA from Matlab. \n");

            beforeKronM = mxGetM(ImageArray);
            beforeKronN = mxGetN(ImageArray);        
            mxSetM(ImageArray, (mwSize)nPixels);
            mxSetN(ImageArray, 1);

            prhs_mCM[0] = ImageArray;
            dims[0] = 4;
            dims[1] = 1;
            prhs_mCM[1] = mxCreateNumericArray(2, dims, mxUINT8_CLASS, mxREAL);
            KronMask = (unsigned char *)mxGetData(prhs_mCM[1]);
            KronMask[0] = 1;
            KronMask[1] = 1;
            KronMask[2] = 1;
            KronMask[3] = 0;
            mexCallMATLAB(1, &ImageArray, 2, prhs_mCM, "kron");
            if (prhs_mCM[0] != prhs[2]) {                                       // Caution
                mxDestroyArray(prhs_mCM[0]);
            }
            else {                                                              // Caution
                mxSetM(prhs_mCM[0], (mwSize)beforeKronM);
                mxSetN(prhs_mCM[0], (mwSize)beforeKronN);
            }
            mxDestroyArray(prhs_mCM[1]);

            Ptr_UINT8 = (unsigned char *)mxGetData(ImageArray) + 3;
            for (i = 0; i < nPixels; i++) {
                *Ptr_UINT8 = 255;
                Ptr_UINT8 += 4;
            }
        } 
    }
    else if (nDim == 3) {
        
        Mode = "RGBA";
        
        Dims3D = mxGetDimensions((const mxArray *)ImageArray);
        
        if (strncmp(Order_string, "HWC", 3) == 0 || strncmp(Order_string, "hwc", 3) == 0) {
            width = (int)Dims3D[1];
            height = (int)Dims3D[0];
            nPixels = (size_t)width * height;
            
            if (*flipImageUD) {
                prhs_mCM[0] = ImageArray;
                dims[0] = 1;
                dims[1] = 1;
                prhs_mCM[1] = mxCreateNumericArray(2, dims, mxUINT32_CLASS, mxREAL);
                FlipMask = (unsigned int *)mxGetData(prhs_mCM[1]);
                FlipMask[0] = 1;
                mexCallMATLAB(1, &ImageArray, 2, prhs_mCM, "flipdim");
                if (prhs_mCM[0] != prhs[2]) {                                   // Caution
                    mxDestroyArray(prhs_mCM[0]);
                }
                mxDestroyArray(prhs_mCM[1]);
            }
            
            if (Dims3D[2] == 3 && strcmp(Mode, "RGBA") == 0) {
                mexPrintf("Data transfer of color image might be faster if passed as RGBA from Matlab. \n");
                Ptr_UINT8 = (unsigned char *)mxCalloc((mwSize)nPixels * 4, sizeof(unsigned char));
                memcpy((void *)Ptr_UINT8, mxGetData(ImageArray), nPixels * 3);
                memset((void *)(Ptr_UINT8 + nPixels * 3), 255, nPixels);
                if (ImageArray != prhs[2]) {                                    // Caution
                    mxDestroyArray(ImageArray);
                }
                dims[0] = 0;
                dims[1] = 1;
                dims[2] = 1;
                ImageArray = mxCreateNumericArray(3, dims, mxUINT8_CLASS, mxREAL);
                mxSetData(ImageArray, (void *)Ptr_UINT8);
                dims[0] = (mwSize)height;
                dims[1] = (mwSize)width;
                dims[2] = 4;
                mxSetDimensions(ImageArray, (const mwSize *)dims, 3);
            }
            
            mexPrintf("Data transfer of image might be faster if passed as CWH from Matlab. \n");
            prhs_mCM[0] = ImageArray;
            dims[0] = 1;
            dims[1] = 3;
            prhs_mCM[1] = mxCreateNumericArray(2, dims, mxDOUBLE_CLASS, mxREAL);
            PermuteMask = (double *)mxGetData(prhs_mCM[1]);
            PermuteMask[0] = 3.0;
            PermuteMask[1] = 2.0;
            PermuteMask[2] = 1.0;
            mexCallMATLAB(1, &ImageArray, 2, prhs_mCM, "permute");
            if (prhs_mCM[0] != prhs[2]) {                                       // Caution
                mxDestroyArray(prhs_mCM[0]);
            }
            mxDestroyArray(prhs_mCM[1]);
        }
        else if (strncmp(Order_string, "CWH", 3) == 0 || strncmp(Order_string, "cwh", 3) == 0) {
            width = (int)Dims3D[1];
            height = (int)Dims3D[2];
            nPixels = (size_t)width * height;
            
            if (*flipImageUD) {
                prhs_mCM[0] = ImageArray;
                dims[0] = 1;
                dims[1] = 1;
                prhs_mCM[1] = mxCreateNumericArray(2, dims, mxUINT32_CLASS, mxREAL);
                FlipMask = (unsigned int *)mxGetData(prhs_mCM[1]);
                FlipMask[0] = 3;
                mexCallMATLAB(1, &ImageArray, 2, prhs_mCM, "flipdim");
                if (prhs_mCM[0] != prhs[2]) {                                   // Caution
                    mxDestroyArray(prhs_mCM[0]);
                }
                mxDestroyArray(prhs_mCM[1]);
            }
            
            if (Dims3D[0] == 3 && strcmp(Mode, "RGBA") == 0) {
                mexPrintf("Data transfer of color image might be faster if passed as RGBA from Matlab. \n");
                Ptr_UINT8 = (unsigned char *)mxCalloc((mwSize)nPixels, sizeof(unsigned char));
                memset((void *)Ptr_UINT8, 255, nPixels);
                dims[0] = 0;
                dims[1] = 1;
                dims[2] = 1;
                AlphaArray = mxCreateNumericArray(3, dims, mxUINT8_CLASS, mxREAL);
                mxSetData(AlphaArray, (void *)Ptr_UINT8);
                dims[0] = 1;
                dims[1] = (mwSize)width;
                dims[2] = (mwSize)height;
                mxSetDimensions(AlphaArray, (const mwSize *)dims, 3);
                
                dims[0] = 1;
                dims[1] = 1;
                prhs_mCM[0] = mxCreateNumericArray(2, dims, mxDOUBLE_CLASS, mxREAL);
                ConcatMask = (double *)mxGetData(prhs_mCM[0]);
                ConcatMask[0] = 1.0;
                prhs_mCM[1] = ImageArray;
                prhs_mCM[2] = AlphaArray;
                mexCallMATLAB(1, &ImageArray, 3, prhs_mCM, "cat");
                mxDestroyArray(prhs_mCM[0]);
                if (prhs_mCM[1] != prhs[2]) {                                   // Caution
                    mxDestroyArray(prhs_mCM[1]);
                }
                mxDestroyArray(prhs_mCM[2]);
            }
        }
        else {
            mexErrMsgTxt("Unrecognized/unsupported ordering.");
        }
    }
    
    MAPPS_ImageArray = mxGetData(ImageArray);

    if (nlhs == 1)
        plhs[0] = ImageArray;

    
    //////////////////////////////////////////////////////////////////////////
	// 3. Connect to the MAPPS shared memory buffer
	//////////////////////////////////////////////////////////////////////////    
    
#include "_SetUpConnection_global_code.h"
#include "_SetUpConnection_client_code.h"
    
    Buffer = ConnectToClient(&h2, client);
	
	if(Buffer == NULL)
	{
		DisconnectFromGlobal(h1, global);
		mexErrMsgIdAndTxt("MEX:UpdateOverlay:UnableToConnectClient", "Unable to connect to shared memory buffer (client state)! \n");
	}


	//////////////////////////////////////////////////////////////////////////
	// 4. Prepare the buffer for use
	//////////////////////////////////////////////////////////////////////////
	
	if(Prepare(global, client, EDS_SetCanvas) == FALSE)
	{
		DisconnectFromClient(h2, client);
        DisconnectFromGlobal(h1, global);
		mexErrMsgIdAndTxt("MEX:UpdateOverlay:ClientBlocked", "Server is still blocking the client channel! \n");
	}
	
    
	//////////////////////////////////////////////////////////////////////////
	// 5. Populate
	//////////////////////////////////////////////////////////////////////////
	
	req = (DS_SetCanvas *)Buffer;
    bufferSize = 0;
    
	sprintf(req->CanvasName, Name_string);
    sprintf(req->Mode, Mode);
	req->Width = width;
	req->Height = height;
    
    bufferSize += sizeof(DS_SetCanvas);
    
    if (strcmp(Mode, "RGBA") == 0)
        memcpy((void *)&(req->Payload), MAPPS_ImageArray, nPixels * 4);
    
    bufferSize += nPixels * 4;
 

	//////////////////////////////////////////////////////////////////////////
	// 6. Execute
	//////////////////////////////////////////////////////////////////////////
	
	if(Execute(global, client, (int)bufferSize) == FALSE)
    {
        DisconnectFromClient(h2, client);
        DisconnectFromGlobal(h1, global);
        mexErrMsgIdAndTxt("MEX:UpdateOverlay:ServerTimedOut", "SERVER TIMED OUT! \n");
    }

	
	//////////////////////////////////////////////////////////////////////////
	// 7. Echo the result
	//////////////////////////////////////////////////////////////////////////
    
    if(strncmp(client->Result, "success", 7) != 0)
	    mexPrintf("Details (mxUpdateOverlay): %s\n", client->Details);
    
    
	//////////////////////////////////////////////////////////////////////////
	// 8. Close shared memory handles
	//////////////////////////////////////////////////////////////////////////

    DisconnectFromClient(h2, client);
    DisconnectFromGlobal(h1, global);
    
    
    //////////////////////////////////////////////////////////////////////////
	// 9. Clean up allocated memory
	//////////////////////////////////////////////////////////////////////////
    
    mxFree(Name_string);
    mxFree(Order_string);
    if (ImageArray != prhs[2] && ImageArray != plhs[0]) {                           // Caution
        mxDestroyArray(ImageArray);
    }
}
