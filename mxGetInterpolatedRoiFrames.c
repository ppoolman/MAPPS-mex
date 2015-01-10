/*=================================================================
 * mxGetInterpolatedRoiFrames.c 
 *
 *=================================================================*/

#include "mxDataIO.h"

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
#include "_SetUpConnection_global_declaration.h"
#include "_SetUpConnection_client_declaration.h"
    
    size_t bufferSize;
    void *Buffer; 
    DS_InterpolatedFrame *req, *resp;
    
    unsigned char *UInt8_Matlab;
    int *Int32_Matlab;
    unsigned int *UInt32_Matlab;    
    int64 *Int64_MAPPS;
    uint64 *UInt64_Matlab;
    float *Float_Matlab;
    double *Double_Matlab;

    
	char *ShapeName, **RoiNames_ptr, *RoiFieldNames_ptr[4], *FrameFieldNames_ptr[16], *WorldObjectFieldNames_ptr[4]; 
    int *IntValue_ptr;
    int64 *Int64Value_ptr;
    float *FloatValue_ptr, *FloatMatrix_ptr, *Vertex_ptr, *Vector_ptr;
    mxArray *RoisStruct_ptr, *Data_ptr;
    mwSize dims[2];
    int nT, nT_maxAllowed, nT_AnyArea, showTypeWarning, i, j, k, nV;
    
    
    //////////////////////////////////////////////////////////////////////////
	// 1. Check for proper number of input and output arguments
	//////////////////////////////////////////////////////////////////////////
    
    if (!(nrhs == 3))
        mexErrMsgTxt("Input argument combination allowed is [struct, scalar, vector].\n");
    
    if (!(!(mxIsEmpty(prhs[0])) && mxIsStruct(prhs[0])))
        mexErrMsgTxt("First input argument is not a struct.\n");
    
    if (!(!(mxIsEmpty(prhs[1])) && mxIsNumeric(prhs[1]) && mxGetNumberOfDimensions(prhs[1]) == 2 && mxGetM(prhs[1]) == 1 && mxGetN(prhs[1]) == 1))
        mexErrMsgTxt("Second input argument is not a scalar.\n");
    
    if (!(!(mxIsEmpty(prhs[2])) && mxIsNumeric(prhs[2]) && mxGetNumberOfDimensions(prhs[2]) == 2 && (mxGetM(prhs[2]) == 1 || mxGetN(prhs[2]) == 1)))
        mexErrMsgTxt("Third input argument is not a vector.\n");
    
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
		mexErrMsgIdAndTxt("MEX:GetInterpolatedRoiFrames:UnableToConnectClient", "Unable to connect to shared memory buffer (client state)! \n");
	}
		
		
	//////////////////////////////////////////////////////////////////////////
	// 3. Prepare the buffer for use
	//////////////////////////////////////////////////////////////////////////
	
	if(Prepare(global, client, EDS_InterpolateRoi) == FALSE)
	{
		DisconnectFromClient(h2, client);
        DisconnectFromGlobal(h1, global);
		mexErrMsgIdAndTxt("MEX:GetInterpolatedRoiFrames:ClientBlocked", "Server is still blocking the client channel! \n");
	}


    //////////////////////////////////////////////////////////////////////////
	// 4. Populate
	//////////////////////////////////////////////////////////////////////////

    req = (DS_InterpolatedFrame *)Buffer;
    req->InputRoiIndex = (int)mxGetScalar(prhs[1]) - 1;
 
    nT = (int)mxGetNumberOfElements(prhs[2]);
    nT_maxAllowed = (int)sizeof(req->InputTimeMillis) / sizeof(int64);
    if (nT > nT_maxAllowed) {
        mexPrintf("\nPLEASE NOTE: Only the first %d requested times will be used...\n\n", nT_maxAllowed);
        nT = nT_maxAllowed;
    }
    Int64_MAPPS = req->InputTimeMillis;
    showTypeWarning = 1;
    switch (mxGetClassID(prhs[2])) {
        case mxUINT8_CLASS:
            UInt8_Matlab = (unsigned char *)mxGetData(prhs[2]); 
            for (j = 0; j < nT; j++)
                *Int64_MAPPS++ = (int64)*UInt8_Matlab++;
            break;
        case mxINT32_CLASS:
            Int32_Matlab = (int *)mxGetData(prhs[2]); 
            for (j = 0; j < nT; j++)
                *Int64_MAPPS++ = (int64)*Int32_Matlab++;
            break;
        case mxUINT32_CLASS:
            UInt32_Matlab = (unsigned int *)mxGetData(prhs[2]); 
            for (j = 0; j < nT; j++)
                *Int64_MAPPS++ = (int64)*UInt32_Matlab++;
            break;
        case mxINT64_CLASS:
            memcpy((void *)Int64_MAPPS, (void *)mxGetData(prhs[2]), (size_t)(nT) * sizeof(int64));
            showTypeWarning = 0;
            break;
        case mxUINT64_CLASS:
            UInt64_Matlab = (uint64 *)mxGetData(prhs[2]); 
            for (j = 0; j < nT; j++)
                *Int64_MAPPS++ = (int64)*UInt64_Matlab++;
            break;
        case mxSINGLE_CLASS:
            Float_Matlab = (float *)mxGetData(prhs[2]); 
            for (j = 0; j < nT; j++)
                *Int64_MAPPS++ = (int64)*Float_Matlab++;
            break;
        case mxDOUBLE_CLASS:
            Double_Matlab = (double *)mxGetData(prhs[2]); 
            for (j = 0; j < nT; j++)
                *Int64_MAPPS++ = (int64)*Double_Matlab++;
            break;
        default:
            mexErrMsgTxt("Unhandled Matlab type for third input argument.\n");
            break;
    }
    if (showTypeWarning == 1)
        mexPrintf("Data transfer for third input argument might be faster if passed as 'int64' from Matlab. \n");
    if (nT < nT_maxAllowed)
        Int64_MAPPS[nT] = -1;
    
    bufferSize = sizeof(DS_InterpolatedFrame);
   	
    
	//////////////////////////////////////////////////////////////////////////
	// 5. Execute
	//////////////////////////////////////////////////////////////////////////
	
    if(Execute(global, client, (int)bufferSize) == FALSE)
    {
        DisconnectFromClient(h2, client);
        DisconnectFromGlobal(h1, global);
        mexErrMsgIdAndTxt("MEX:GetInterpolatedRoiFrames:ServerTimedOut", "SERVER TIMED OUT! \n");
    }


	//////////////////////////////////////////////////////////////////////////
	// 6. Echo the result
	//////////////////////////////////////////////////////////////////////////

	if(strncmp(client->Result, "success", 7) == 0)
    {
        resp = (DS_InterpolatedFrame *)Buffer;
        if (nlhs == 0)
            {
            switch(resp->Shape)
            {
                case ShapeUnknown:
                    ShapeName = "Unknown shape";
                    break;
                case ShapeRectangle:
                    ShapeName = "Rectangle";
                    break;
                case ShapeEllipse:
                    ShapeName = "Ellipse";
                    break;
                case ShapeFree:
                    ShapeName = "Free form";
                    break;
                case ShapePoints:
                    ShapeName = "Polygon";
                    break;
                default:
                    ShapeName = "Unspecified shape (ERROR)";
                    break;
            }
            if (nT == 1)
            {
                if (AnyArea(&resp->Frames[0]))
                {
                    if (resp->ObjectIndex > -1)
                        mexPrintf("  '%s': %s on canvas %d (world object %d)\n",  
                                  resp->Name,
                                  ShapeName,
                                  resp->ScreenIndex + 1,
                                  resp->ObjectIndex + 1);
                    else
                        mexPrintf("  '%s': %s on canvas %d (no associated world object)\n",  
                                  resp->Name,
                                  ShapeName,
                                  resp->ScreenIndex + 1);
                }
                else
                {
                    mexPrintf("  '%s': does not exist on canvas %d for requested time\n",  
                              resp->Name,
                              resp->ScreenIndex + 1);
                }
            }
            else
            {
                nT_AnyArea = 0;
                for (j = 0; j < nT; j++)
                {
                    if (AnyArea(&resp->Frames[j]))
                        nT_AnyArea += 1;
                }
          
                if (nT_AnyArea > 0)
                {
                    if (resp->ObjectIndex > -1)
                        mexPrintf("  '%s': %s (%d frames) on canvas %d (world object %d)\n",  
                                  resp->Name,
                                  ShapeName,
                                  nT_AnyArea,
                                  resp->ScreenIndex + 1,
                                  resp->ObjectIndex + 1);
                    else
                        mexPrintf("  '%s': %s (%d frames) on canvas %d (no associated world object)\n",  
                                  resp->Name,
                                  ShapeName,
                                  nT_AnyArea,
                                  resp->ScreenIndex + 1);
                }
                else
                {
                    mexPrintf("  '%s': does not exist on canvas %d for requested times\n",  
                              resp->Name,
                              resp->ScreenIndex + 1);
                }
            }
        }
	}
	else
	{
		mexPrintf("Details (mxGetInterpolatedRoiFrames): %s\n", client->Details);
	}
    
    
    //////////////////////////////////////////////////////////////////////////
	// 7. Populate Matlab output
	//////////////////////////////////////////////////////////////////////////
    
    if (nlhs == 1) {
        if (strncmp(client->Result, "success", 7) == 0) {
            RoiNames_ptr = (char **)mxCalloc(1, (mwSize)sizeof(char *));
            RoiNames_ptr[0] = resp->Name;
            dims[0] = 1;
            dims[1] = 1;
            RoisStruct_ptr = mxCreateStructArray(2, dims, 1, (const char **)RoiNames_ptr);

            RoiFieldNames_ptr[0] = "CanvasIndex";
            RoiFieldNames_ptr[1] = "Shape";
            RoiFieldNames_ptr[2] = "Frame";
            RoiFieldNames_ptr[3] = "WorldObject";            

            i = 0;
            
            dims[0] = 1;
            dims[1] = 1;
            //void mxSetField(mxArray *pm, mwIndex index, const char *fieldname, mxArray *pvalue);
            //mxSetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i], mxCreateStructArray(2, dims, 3, (const char **)RoiFieldNames_ptr));                
            //void mxSetFieldByNumber(mxArray *pm, mwIndex index, int fieldnumber, mxArray *pvalue);
            mxSetFieldByNumber(RoisStruct_ptr, 0, i, mxCreateStructArray(2, dims, 4, (const char **)RoiFieldNames_ptr));

            IntValue_ptr = (int *)mxCalloc(1, (mwSize)sizeof(int));
            *IntValue_ptr = resp->ScreenIndex + 1;
            dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
            dims[1] = 1;
            Data_ptr = mxCreateNumericArray(2, dims, mxINT32_CLASS, mxREAL);
            mxSetM(Data_ptr, 1);
            mxSetData(Data_ptr, IntValue_ptr); 
            //mxArray *mxGetField(const mxArray *pm, mwIndex index, const char *fieldname);
            //mxSetField(mxGetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i]), 0, (const char *)RoiFieldNames_ptr[0], Data_ptr);
            //mxArray *mxGetFieldByNumber(const mxArray *pm, mwIndex index, int fieldnumber);
            mxSetField(mxGetFieldByNumber(RoisStruct_ptr, 0, i), 0, (const char *)RoiFieldNames_ptr[0], Data_ptr);

            switch (resp->Shape)
            {
                case ShapeUnknown:
                    Data_ptr = mxCreateString("unknown");
                    break;
                case ShapeRectangle:
                    Data_ptr = mxCreateString("rectangle");
                    break;
                case ShapeEllipse:
                    Data_ptr = mxCreateString("ellipse");
                    break;
                case ShapeFree:
                    Data_ptr = mxCreateString("free-form polygon");
                    break;
                case ShapePoints:
                    Data_ptr = mxCreateString("polygon");
                    break;
                default:
                    Data_ptr = mxCreateString("unspecified");
                    break;
            }
            //mxSetField(mxGetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i]), 0, (const char *)RoiFieldNames_ptr[1], Data_ptr);
            mxSetField(mxGetFieldByNumber(RoisStruct_ptr, 0, i), 0, (const char *)RoiFieldNames_ptr[1], Data_ptr);

            switch (resp->Shape)
            {
                case ShapeUnknown:
                    for (j = 0; j < nT; j++) {

                    }
                    break;
                case ShapeRectangle:
                    FrameFieldNames_ptr[0] = "MAPPS_time";
                    FrameFieldNames_ptr[1] = "X0";
                    FrameFieldNames_ptr[2] = "Y0";
                    FrameFieldNames_ptr[3] = "X1";
                    FrameFieldNames_ptr[4] = "Y1";
                    FrameFieldNames_ptr[5] = "Width";
                    FrameFieldNames_ptr[6] = "Height";
                    dims[0] = nT;
                    dims[1] = 1;
                    //mxSetField(mxGetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i]), 0, (const char *)RoiFieldNames_ptr[2], mxCreateStructArray(2, dims, 7, (const char **)FrameFieldNames_ptr));
                    mxSetField(mxGetFieldByNumber(RoisStruct_ptr, 0, i), 0, (const char *)RoiFieldNames_ptr[2], mxCreateStructArray(2, dims, 7, (const char **)FrameFieldNames_ptr));
                    for (j = 0; j < nT; j++) {
                        Int64Value_ptr = (int64 *)mxCalloc(1, (mwSize)sizeof(int64));
                        *Int64Value_ptr = (int64)(resp->Frames[j].TimeInSec * 1000.0);
                        dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
                        dims[1] = 1;
                        Data_ptr = mxCreateNumericArray(2, dims, mxINT64_CLASS, mxREAL);
                        mxSetM(Data_ptr, 1);
                        mxSetData(Data_ptr, Int64Value_ptr); 
                        //mxSetField(mxGetField(mxGetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i]), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[0], Data_ptr);
                        mxSetField(mxGetField(mxGetFieldByNumber(RoisStruct_ptr, 0, i), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[0], Data_ptr);
                        
                        if (AnyArea(&resp->Frames[j])) {
                            FloatValue_ptr = (float *)mxCalloc(1, (mwSize)sizeof(float));
                            *FloatValue_ptr = resp->Frames[j].CanvasX0;
                            dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
                            dims[1] = 1;
                            Data_ptr = mxCreateNumericArray(2, dims, mxSINGLE_CLASS, mxREAL);
                            mxSetM(Data_ptr, 1);
                            mxSetData(Data_ptr, FloatValue_ptr); 
                            //mxSetField(mxGetField(mxGetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i]), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[1], Data_ptr);
                            mxSetField(mxGetField(mxGetFieldByNumber(RoisStruct_ptr, 0, i), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[1], Data_ptr);

                            FloatValue_ptr = (float *)mxCalloc(1, (mwSize)sizeof(float));
                            *FloatValue_ptr = resp->Frames[j].CanvasY0;
                            dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
                            dims[1] = 1;
                            Data_ptr = mxCreateNumericArray(2, dims, mxSINGLE_CLASS, mxREAL);
                            mxSetM(Data_ptr, 1);
                            mxSetData(Data_ptr, FloatValue_ptr); 
                            //mxSetField(mxGetField(mxGetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i]), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[2], Data_ptr);
                            mxSetField(mxGetField(mxGetFieldByNumber(RoisStruct_ptr, 0, i), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[2], Data_ptr);

                            FloatValue_ptr = (float *)mxCalloc(1, (mwSize)sizeof(float));
                            *FloatValue_ptr = resp->Frames[j].CanvasX1;
                            dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
                            dims[1] = 1;
                            Data_ptr = mxCreateNumericArray(2, dims, mxSINGLE_CLASS, mxREAL);
                            mxSetM(Data_ptr, 1);
                            mxSetData(Data_ptr, FloatValue_ptr); 
                            //mxSetField(mxGetField(mxGetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i]), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[3], Data_ptr);
                            mxSetField(mxGetField(mxGetFieldByNumber(RoisStruct_ptr, 0, i), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[3], Data_ptr);

                            FloatValue_ptr = (float *)mxCalloc(1, (mwSize)sizeof(float));
                            *FloatValue_ptr = resp->Frames[j].CanvasY1;
                            dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
                            dims[1] = 1;
                            Data_ptr = mxCreateNumericArray(2, dims, mxSINGLE_CLASS, mxREAL);
                            mxSetM(Data_ptr, 1);
                            mxSetData(Data_ptr, FloatValue_ptr); 
                            //mxSetField(mxGetField(mxGetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i]), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[4], Data_ptr);
                            mxSetField(mxGetField(mxGetFieldByNumber(RoisStruct_ptr, 0, i), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[4], Data_ptr);

                            FloatValue_ptr = (float *)mxCalloc(1, (mwSize)sizeof(float));
                            *FloatValue_ptr = resp->Frames[j].CanvasWidth;
                            dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
                            dims[1] = 1;
                            Data_ptr = mxCreateNumericArray(2, dims, mxSINGLE_CLASS, mxREAL);
                            mxSetM(Data_ptr, 1);
                            mxSetData(Data_ptr, FloatValue_ptr); 
                            //mxSetField(mxGetField(mxGetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i]), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[5], Data_ptr);
                            mxSetField(mxGetField(mxGetFieldByNumber(RoisStruct_ptr, 0, i), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[5], Data_ptr);

                            FloatValue_ptr = (float *)mxCalloc(1, (mwSize)sizeof(float));
                            *FloatValue_ptr = resp->Frames[j].CanvasHeight;
                            dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
                            dims[1] = 1;
                            Data_ptr = mxCreateNumericArray(2, dims, mxSINGLE_CLASS, mxREAL);
                            mxSetM(Data_ptr, 1);
                            mxSetData(Data_ptr, FloatValue_ptr); 
                            //mxSetField(mxGetField(mxGetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i]), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[6], Data_ptr);
                            mxSetField(mxGetField(mxGetFieldByNumber(RoisStruct_ptr, 0, i), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[6], Data_ptr);
                        }
                    }
                    break;
                case ShapeEllipse:
                    FrameFieldNames_ptr[0] = "MAPPS_time";
                    FrameFieldNames_ptr[1] = "Xm";
                    FrameFieldNames_ptr[2] = "Ym";
                    FrameFieldNames_ptr[3] = "RadiusX";
                    FrameFieldNames_ptr[4] = "RadiusY";
                    FrameFieldNames_ptr[5] = "Width";
                    FrameFieldNames_ptr[6] = "Height";
                    dims[0] = nT;
                    dims[1] = 1;
                    //mxSetField(mxGetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i]), 0, (const char *)RoiFieldNames_ptr[2], mxCreateStructArray(2, dims, 7, (const char **)FrameFieldNames_ptr));
                    mxSetField(mxGetFieldByNumber(RoisStruct_ptr, 0, i), 0, (const char *)RoiFieldNames_ptr[2], mxCreateStructArray(2, dims, 7, (const char **)FrameFieldNames_ptr));
                    for (j = 0; j < nT; j++) {
                        Int64Value_ptr = (int64 *)mxCalloc(1, (mwSize)sizeof(int64));
                        *Int64Value_ptr = (int64)(resp->Frames[j].TimeInSec * 1000.0);
                        dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
                        dims[1] = 1;
                        Data_ptr = mxCreateNumericArray(2, dims, mxINT64_CLASS, mxREAL);
                        mxSetM(Data_ptr, 1);
                        mxSetData(Data_ptr, Int64Value_ptr); 
                        //mxSetField(mxGetField(mxGetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i]), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[0], Data_ptr);
                        mxSetField(mxGetField(mxGetFieldByNumber(RoisStruct_ptr, 0, i), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[0], Data_ptr);

                        if (AnyArea(&resp->Frames[j])) {
                            FloatValue_ptr = (float *)mxCalloc(1, (mwSize)sizeof(float));
                            *FloatValue_ptr = resp->Frames[j].CanvasXm;
                            dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
                            dims[1] = 1;
                            Data_ptr = mxCreateNumericArray(2, dims, mxSINGLE_CLASS, mxREAL);
                            mxSetM(Data_ptr, 1);
                            mxSetData(Data_ptr, FloatValue_ptr); 
                            //mxSetField(mxGetField(mxGetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i]), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[1], Data_ptr);
                            mxSetField(mxGetField(mxGetFieldByNumber(RoisStruct_ptr, 0, i), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[1], Data_ptr);

                            FloatValue_ptr = (float *)mxCalloc(1, (mwSize)sizeof(float));
                            *FloatValue_ptr = resp->Frames[j].CanvasYm;
                            dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
                            dims[1] = 1;
                            Data_ptr = mxCreateNumericArray(2, dims, mxSINGLE_CLASS, mxREAL);
                            mxSetM(Data_ptr, 1);
                            mxSetData(Data_ptr, FloatValue_ptr); 
                            //mxSetField(mxGetField(mxGetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i]), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[2], Data_ptr);
                            mxSetField(mxGetField(mxGetFieldByNumber(RoisStruct_ptr, 0, i), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[2], Data_ptr);

                            FloatValue_ptr = (float *)mxCalloc(1, (mwSize)sizeof(float));
                            *FloatValue_ptr = resp->Frames[j].CanvasRadiusX;
                            dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
                            dims[1] = 1;
                            Data_ptr = mxCreateNumericArray(2, dims, mxSINGLE_CLASS, mxREAL);
                            mxSetM(Data_ptr, 1);
                            mxSetData(Data_ptr, FloatValue_ptr); 
                            //mxSetField(mxGetField(mxGetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i]), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[3], Data_ptr);
                            mxSetField(mxGetField(mxGetFieldByNumber(RoisStruct_ptr, 0, i), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[3], Data_ptr);

                            FloatValue_ptr = (float *)mxCalloc(1, (mwSize)sizeof(float));
                            *FloatValue_ptr = resp->Frames[j].CanvasRadiusY;
                            dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
                            dims[1] = 1;
                            Data_ptr = mxCreateNumericArray(2, dims, mxSINGLE_CLASS, mxREAL);
                            mxSetM(Data_ptr, 1);
                            mxSetData(Data_ptr, FloatValue_ptr); 
                            //mxSetField(mxGetField(mxGetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i]), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[4], Data_ptr);
                            mxSetField(mxGetField(mxGetFieldByNumber(RoisStruct_ptr, 0, i), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[4], Data_ptr);

                            FloatValue_ptr = (float *)mxCalloc(1, (mwSize)sizeof(float));
                            *FloatValue_ptr = resp->Frames[j].CanvasWidth;
                            dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
                            dims[1] = 1;
                            Data_ptr = mxCreateNumericArray(2, dims, mxSINGLE_CLASS, mxREAL);
                            mxSetM(Data_ptr, 1);
                            mxSetData(Data_ptr, FloatValue_ptr); 
                            //mxSetField(mxGetField(mxGetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i]), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[5], Data_ptr);
                            mxSetField(mxGetField(mxGetFieldByNumber(RoisStruct_ptr, 0, i), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[5], Data_ptr);

                            FloatValue_ptr = (float *)mxCalloc(1, (mwSize)sizeof(float));
                            *FloatValue_ptr = resp->Frames[j].CanvasHeight;
                            dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
                            dims[1] = 1;
                            Data_ptr = mxCreateNumericArray(2, dims, mxSINGLE_CLASS, mxREAL);
                            mxSetM(Data_ptr, 1);
                            mxSetData(Data_ptr, FloatValue_ptr); 
                            //mxSetField(mxGetField(mxGetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i]), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[6], Data_ptr);
                            mxSetField(mxGetField(mxGetFieldByNumber(RoisStruct_ptr, 0, i), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[6], Data_ptr);
                        }
                    }
                    break;
                case ShapeFree:

                case ShapePoints:
                    FrameFieldNames_ptr[0] = "MAPPS_time";
                    FrameFieldNames_ptr[1] = "Vertices";
                    FrameFieldNames_ptr[2] = "Width";
                    FrameFieldNames_ptr[3] = "Height";
                    dims[0] = nT;
                    dims[1] = 1;
                    //mxSetField(mxGetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i]), 0, (const char *)RoiFieldNames_ptr[2], mxCreateStructArray(2, dims, 5, (const char **)FrameFieldNames_ptr));
                    mxSetField(mxGetFieldByNumber(RoisStruct_ptr, 0, i), 0, (const char *)RoiFieldNames_ptr[2], mxCreateStructArray(2, dims, 5, (const char **)FrameFieldNames_ptr));
                    for (j = 0; j < nT; j++) {
                        Int64Value_ptr = (int64 *)mxCalloc(1, (mwSize)sizeof(int64));
                        *Int64Value_ptr = (int64)(resp->Frames[j].TimeInSec * 1000.0);
                        dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
                        dims[1] = 1;
                        Data_ptr = mxCreateNumericArray(2, dims, mxINT64_CLASS, mxREAL);
                        mxSetM(Data_ptr, 1);
                        mxSetData(Data_ptr, Int64Value_ptr); 
                        //mxSetField(mxGetField(mxGetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i]), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[0], Data_ptr);
                        mxSetField(mxGetField(mxGetFieldByNumber(RoisStruct_ptr, 0, i), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[0], Data_ptr);

                        if (AnyArea(&resp->Frames[j])) {
                            /*
                            IntValue_ptr = (int *)mxCalloc(1, (mwSize)sizeof(int));
                            *IntValue_ptr = resp->Frame.VertexCount;
                            dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
                            dims[1] = 1;
                            Data_ptr = mxCreateNumericArray(2, dims, mxINT32_CLASS, mxREAL);
                            mxSetM(Data_ptr, 1);
                            mxSetData(Data_ptr, IntValue_ptr); 
                            //mxSetField(mxGetField(mxGetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i]), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[1], Data_ptr);
                            mxSetField(mxGetField(mxGetFieldByNumber(RoisStruct_ptr, 0, i), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[1], Data_ptr);
                            */
                            nV = resp->Frames[j].VertexCount;
                            FloatMatrix_ptr = (float *)mxCalloc((mwSize)nV * 2, (mwSize)sizeof(float));
                            FloatValue_ptr = FloatMatrix_ptr;
                            Vertex_ptr = resp->Frames[j].FreeVerticies;
                            for (k = 0; k < nV; k++) {
                                *FloatValue_ptr = *Vertex_ptr++;
                                FloatValue_ptr[nV] = *Vertex_ptr++;
                                FloatValue_ptr++;
                            }
                            dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
                            dims[1] = 1;
                            Data_ptr = mxCreateNumericArray(2, dims, mxSINGLE_CLASS, mxREAL);
                            mxSetM(Data_ptr, (mwSize)nV);
                            mxSetN(Data_ptr, 2);
                            mxSetData(Data_ptr, FloatMatrix_ptr);                            
                            //mxSetField(mxGetField(mxGetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i]), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[1], Data_ptr);
                            mxSetField(mxGetField(mxGetFieldByNumber(RoisStruct_ptr, 0, i), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[1], Data_ptr);

                            FloatValue_ptr = (float *)mxCalloc(1, (mwSize)sizeof(float));
                            *FloatValue_ptr = resp->Frames[j].CanvasWidth;
                            dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
                            dims[1] = 1;
                            Data_ptr = mxCreateNumericArray(2, dims, mxSINGLE_CLASS, mxREAL);
                            mxSetM(Data_ptr, 1);
                            mxSetData(Data_ptr, FloatValue_ptr); 
                            //mxSetField(mxGetField(mxGetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i]), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[2], Data_ptr);
                            mxSetField(mxGetField(mxGetFieldByNumber(RoisStruct_ptr, 0, i), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[2], Data_ptr);

                            FloatValue_ptr = (float *)mxCalloc(1, (mwSize)sizeof(float));
                            *FloatValue_ptr = resp->Frames[j].CanvasHeight;
                            dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
                            dims[1] = 1;
                            Data_ptr = mxCreateNumericArray(2, dims, mxSINGLE_CLASS, mxREAL);
                            mxSetM(Data_ptr, 1);
                            mxSetData(Data_ptr, FloatValue_ptr); 
                            //mxSetField(mxGetField(mxGetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i]), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[3], Data_ptr);
                            mxSetField(mxGetField(mxGetFieldByNumber(RoisStruct_ptr, 0, i), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[3], Data_ptr);
                        }
                    }
                    break;
                default:

                    break;
            }
            
            WorldObjectFieldNames_ptr[0] = "Index";
            WorldObjectFieldNames_ptr[1] = "Origin";
            WorldObjectFieldNames_ptr[2] = "XVector";
            WorldObjectFieldNames_ptr[3] = "YVector";
            dims[0] = 1;
            dims[1] = 1;
            //mxSetField(mxGetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i]), 0, (const char *)RoiFieldNames_ptr[3], mxCreateStructArray(2, dims, 4, (const char **)WorldObjectFieldNames_ptr));
            mxSetField(mxGetFieldByNumber(RoisStruct_ptr, 0, i), 0, (const char *)RoiFieldNames_ptr[3], mxCreateStructArray(2, dims, 4, (const char **)WorldObjectFieldNames_ptr));

            if (resp->ObjectIndex > -1) {
                j = 0;
                
                IntValue_ptr = (int *)mxCalloc(1, (mwSize)sizeof(int));
                *IntValue_ptr = resp->ObjectIndex + 1;
                dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
                dims[1] = 1;
                Data_ptr = mxCreateNumericArray(2, dims, mxINT32_CLASS, mxREAL);
                mxSetM(Data_ptr, 1);
                mxSetData(Data_ptr, IntValue_ptr); 
                //mxSetField(mxGetField(mxGetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i]), 0, RoiFieldNames_ptr[3]), (mwIndex)j, WorldObjectFieldNames_ptr[0], Data_ptr);
                mxSetField(mxGetField(mxGetFieldByNumber(RoisStruct_ptr, 0, i), 0, RoiFieldNames_ptr[3]), (mwIndex)j, WorldObjectFieldNames_ptr[0], Data_ptr);
                
                FloatMatrix_ptr = (float *)mxCalloc(3, (mwSize)sizeof(float));
                FloatValue_ptr = FloatMatrix_ptr;
                Vector_ptr = resp->ObjectOrigin;
                for (k = 0; k < 3; k++)
                    *FloatValue_ptr++ = *Vector_ptr++;
                dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
                dims[1] = 1;
                Data_ptr = mxCreateNumericArray(2, dims, mxSINGLE_CLASS, mxREAL);
                mxSetM(Data_ptr, 1);
                mxSetN(Data_ptr, 3);
                mxSetData(Data_ptr, FloatMatrix_ptr);                            
                //mxSetField(mxGetField(mxGetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i]), 0, RoiFieldNames_ptr[3]), (mwIndex)j, WorldObjectFieldNames_ptr[1], Data_ptr);
                mxSetField(mxGetField(mxGetFieldByNumber(RoisStruct_ptr, 0, i), 0, RoiFieldNames_ptr[3]), (mwIndex)j, WorldObjectFieldNames_ptr[1], Data_ptr);
                
                FloatMatrix_ptr = (float *)mxCalloc(3, (mwSize)sizeof(float));
                FloatValue_ptr = FloatMatrix_ptr;
                Vector_ptr = resp->ObjectXVector;
                for (k = 0; k < 3; k++)
                    *FloatValue_ptr++ = *Vector_ptr++;
                dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
                dims[1] = 1;
                Data_ptr = mxCreateNumericArray(2, dims, mxSINGLE_CLASS, mxREAL);
                mxSetM(Data_ptr, 1);
                mxSetN(Data_ptr, 3);
                mxSetData(Data_ptr, FloatMatrix_ptr);                            
                //mxSetField(mxGetField(mxGetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i]), 0, RoiFieldNames_ptr[3]), (mwIndex)j, WorldObjectFieldNames_ptr[2], Data_ptr);
                mxSetField(mxGetField(mxGetFieldByNumber(RoisStruct_ptr, 0, i), 0, RoiFieldNames_ptr[3]), (mwIndex)j, WorldObjectFieldNames_ptr[2], Data_ptr);
                
                FloatMatrix_ptr = (float *)mxCalloc(3, (mwSize)sizeof(float));
                FloatValue_ptr = FloatMatrix_ptr;
                Vector_ptr = resp->ObjectYVector;
                for (k = 0; k < 3; k++)
                    *FloatValue_ptr++ = *Vector_ptr++;
                dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
                dims[1] = 1;
                Data_ptr = mxCreateNumericArray(2, dims, mxSINGLE_CLASS, mxREAL);
                mxSetM(Data_ptr, 1);
                mxSetN(Data_ptr, 3);
                mxSetData(Data_ptr, FloatMatrix_ptr);                            
                //mxSetField(mxGetField(mxGetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i]), 0, RoiFieldNames_ptr[3]), (mwIndex)j, WorldObjectFieldNames_ptr[3], Data_ptr);
                mxSetField(mxGetField(mxGetFieldByNumber(RoisStruct_ptr, 0, i), 0, RoiFieldNames_ptr[3]), (mwIndex)j, WorldObjectFieldNames_ptr[3], Data_ptr);
            }
                
            plhs[0] = RoisStruct_ptr;

            mxFree(RoiNames_ptr);
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
