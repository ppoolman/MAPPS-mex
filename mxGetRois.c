/*=================================================================
 * mxGetRois.c 
 *
 *=================================================================*/

#include "mxDataIO.h"

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
#include "_SetUpConnection_global_declaration.h"
#include "_SetUpConnection_client_declaration.h"
    
    size_t bufferSize;
    void *Buffer; 
    int *RequestedRoiIndex;
    DS_RegionsOfInterest *resp;
    
	char *ShapeName, **RoiNames_ptr, *RoiFieldNames_ptr[3], *FrameFieldNames_ptr[16]; 
    int *IntValue_ptr;
    int64 *Int64Value_ptr;
    float *FloatValue_ptr, *FloatMatrix_ptr, *Vertex_ptr;
    mxArray *RoisStruct_ptr, *Data_ptr;
    mwSize dims[2];
    int i, j, k, nV;
    
    
    //////////////////////////////////////////////////////////////////////////
	// 1. Check for proper number of input and output arguments
	//////////////////////////////////////////////////////////////////////////
    
    if (!(nrhs == 1 || nrhs == 2))
        mexErrMsgTxt("Input argument combinations allowed are [struct], or [struct, scalar].\n");
    
    if (!(!(mxIsEmpty(prhs[0])) && mxIsStruct(prhs[0])))
        mexErrMsgTxt("First input argument is not a struct.\n");
    
    if (nrhs == 2)
    {    
        if (!(!(mxIsEmpty(prhs[1])) && mxIsNumeric(prhs[1]) && mxGetNumberOfDimensions(prhs[1]) == 2 && mxGetM(prhs[1]) == 1 && mxGetN(prhs[1]) == 1))
            mexErrMsgTxt("Second input argument is not a scalar.\n");
    }
    
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
		mexErrMsgIdAndTxt("MEX:GetRois:UnableToConnectClient", "Unable to connect to shared memory buffer (client state)! \n");
	}
		
		
	//////////////////////////////////////////////////////////////////////////
	// 3. Prepare the buffer for use
	//////////////////////////////////////////////////////////////////////////
	
	if(Prepare(global, client, EDS_GetRoi) == FALSE)
	{
		DisconnectFromClient(h2, client);
        DisconnectFromGlobal(h1, global);
		mexErrMsgIdAndTxt("MEX:GetRois:ClientBlocked", "Server is still blocking the client channel! \n");
	}


    //////////////////////////////////////////////////////////////////////////
	// 4. Populate
	//////////////////////////////////////////////////////////////////////////

    RequestedRoiIndex = (int *)Buffer;
    if (nrhs == 2)
        *RequestedRoiIndex = (int)mxGetScalar(prhs[1]) - 1;
    else
        *RequestedRoiIndex = -1;
    bufferSize = sizeof(int);
   	
    
	//////////////////////////////////////////////////////////////////////////
	// 5. Execute
	//////////////////////////////////////////////////////////////////////////
	
    if(Execute(global, client, (int)bufferSize) == FALSE)
    {
        DisconnectFromClient(h2, client);
        DisconnectFromGlobal(h1, global);
        mexErrMsgIdAndTxt("MEX:GetRois:ServerTimedOut", "SERVER TIMED OUT! \n");
    }


	//////////////////////////////////////////////////////////////////////////
	// 6. Echo the result
	//////////////////////////////////////////////////////////////////////////

	if(strncmp(client->Result, "success", 7) == 0)
    {
        resp = (DS_RegionsOfInterest *)Buffer;
        if (nlhs == 0) {
            if (nrhs != 2 || resp->Count > 1)
                mexPrintf("There %s %d %s \n", 
                          resp->Count == 1 ? "is" : "are",
                          resp->Count, 
                          resp->Count == 1 ? "ROI:" : "ROIs:");

            for(i=0; i<resp->Count; i++)
            {
                switch(resp->Roi[i].Shape)
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
                if (nrhs == 2 && resp->Count == 1)
                    mexPrintf("  [%d - '%s']: %s -- %s on canvas %d \n", 
                              (int)mxGetScalar(prhs[1]), 
                              resp->Roi[i].Name,
                              ShapeName,
                              resp->Roi[i].FrameCount == 1 ? "static" : "dynamic",
                              resp->Roi[i].ScreenIndex + 1);
                else
                    mexPrintf("  [%d - '%s']: %s -- %s on canvas %d \n", 
                              i+1, 
                              resp->Roi[i].Name,
                              ShapeName,
                              resp->Roi[i].FrameCount == 1 ? "static" : "dynamic",
                              resp->Roi[i].ScreenIndex + 1);
            }
        }
	}
	else
	{
		mexPrintf("Details (mxGetRois): %s\n", client->Details);
	}
    
    
    //////////////////////////////////////////////////////////////////////////
	// 7. Populate Matlab output
	//////////////////////////////////////////////////////////////////////////
    
    if (nlhs == 1) {
        if (strncmp(client->Result, "success", 7) == 0) {
            RoiNames_ptr = (char **)mxCalloc((mwSize)resp->Count, (mwSize)sizeof(char *));
            for (i = 0; i < resp->Count; i++) {
                RoiNames_ptr[i] = resp->Roi[i].Name;
            }
            dims[0] = 1;
            dims[1] = 1;
            RoisStruct_ptr = mxCreateStructArray(2, dims, (int)resp->Count, (const char **)RoiNames_ptr);

            RoiFieldNames_ptr[0] = "CanvasIndex";
            RoiFieldNames_ptr[1] = "Shape";
            RoiFieldNames_ptr[2] = "Frame";            
            for (i = 0; i < resp->Count; i++) {
                dims[0] = 1;
                dims[1] = 1;
                //void mxSetField(mxArray *pm, mwIndex index, const char *fieldname, mxArray *pvalue);
                //mxSetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i], mxCreateStructArray(2, dims, 3, (const char **)RoiFieldNames_ptr));                
                //void mxSetFieldByNumber(mxArray *pm, mwIndex index, int fieldnumber, mxArray *pvalue);
                mxSetFieldByNumber(RoisStruct_ptr, 0, i, mxCreateStructArray(2, dims, 3, (const char **)RoiFieldNames_ptr));
                
                IntValue_ptr = (int *)mxCalloc(1, (mwSize)sizeof(int));
                *IntValue_ptr = resp->Roi[i].ScreenIndex + 1;
                dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
                dims[1] = 1;
                Data_ptr = mxCreateNumericArray(2, dims, mxINT32_CLASS, mxREAL);
                mxSetM(Data_ptr, 1);
                mxSetData(Data_ptr, IntValue_ptr); 
                //mxArray *mxGetField(const mxArray *pm, mwIndex index, const char *fieldname);
                //mxSetField(mxGetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i]), 0, (const char *)RoiFieldNames_ptr[0], Data_ptr);
                //mxArray *mxGetFieldByNumber(const mxArray *pm, mwIndex index, int fieldnumber);
                mxSetField(mxGetFieldByNumber(RoisStruct_ptr, 0, i), 0, (const char *)RoiFieldNames_ptr[0], Data_ptr);
                
                switch (resp->Roi[i].Shape)
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

                switch (resp->Roi[i].Shape)
                {
                    case ShapeUnknown:
                        for (j = 0; j < resp->Roi[i].FrameCount; j++) {
                            
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
                        dims[0] = (mwSize)resp->Roi[i].FrameCount;
                        dims[1] = 1;
                        //mxSetField(mxGetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i]), 0, (const char *)RoiFieldNames_ptr[2], mxCreateStructArray(2, dims, 7, (const char **)FrameFieldNames_ptr));
                        mxSetField(mxGetFieldByNumber(RoisStruct_ptr, 0, i), 0, (const char *)RoiFieldNames_ptr[2], mxCreateStructArray(2, dims, 7, (const char **)FrameFieldNames_ptr));
                        for (j = 0; j < resp->Roi[i].FrameCount; j++) {
                            Int64Value_ptr = (int64 *)mxCalloc(1, (mwSize)sizeof(int64));
                            *Int64Value_ptr = (int64)(resp->Roi[i].AllFrames[j].TimeInSec * 1000.0);
                            dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
                            dims[1] = 1;
                            Data_ptr = mxCreateNumericArray(2, dims, mxINT64_CLASS, mxREAL);
                            mxSetM(Data_ptr, 1);
                            mxSetData(Data_ptr, Int64Value_ptr); 
                            //mxSetField(mxGetField(mxGetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i]), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[0], Data_ptr);
                            mxSetField(mxGetField(mxGetFieldByNumber(RoisStruct_ptr, 0, i), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[0], Data_ptr);
                            
                            FloatValue_ptr = (float *)mxCalloc(1, (mwSize)sizeof(float));
                            *FloatValue_ptr = resp->Roi[i].AllFrames[j].CanvasX0;
                            dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
                            dims[1] = 1;
                            Data_ptr = mxCreateNumericArray(2, dims, mxSINGLE_CLASS, mxREAL);
                            mxSetM(Data_ptr, 1);
                            mxSetData(Data_ptr, FloatValue_ptr); 
                            //mxSetField(mxGetField(mxGetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i]), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[1], Data_ptr);
                            mxSetField(mxGetField(mxGetFieldByNumber(RoisStruct_ptr, 0, i), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[1], Data_ptr);
                            
                            FloatValue_ptr = (float *)mxCalloc(1, (mwSize)sizeof(float));
                            *FloatValue_ptr = resp->Roi[i].AllFrames[j].CanvasY0;
                            dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
                            dims[1] = 1;
                            Data_ptr = mxCreateNumericArray(2, dims, mxSINGLE_CLASS, mxREAL);
                            mxSetM(Data_ptr, 1);
                            mxSetData(Data_ptr, FloatValue_ptr); 
                            //mxSetField(mxGetField(mxGetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i]), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[2], Data_ptr);
                            mxSetField(mxGetField(mxGetFieldByNumber(RoisStruct_ptr, 0, i), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[2], Data_ptr);
                            
                            FloatValue_ptr = (float *)mxCalloc(1, (mwSize)sizeof(float));
                            *FloatValue_ptr = resp->Roi[i].AllFrames[j].CanvasX1;
                            dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
                            dims[1] = 1;
                            Data_ptr = mxCreateNumericArray(2, dims, mxSINGLE_CLASS, mxREAL);
                            mxSetM(Data_ptr, 1);
                            mxSetData(Data_ptr, FloatValue_ptr); 
                            //mxSetField(mxGetField(mxGetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i]), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[3], Data_ptr);
                            mxSetField(mxGetField(mxGetFieldByNumber(RoisStruct_ptr, 0, i), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[3], Data_ptr);
                            
                            FloatValue_ptr = (float *)mxCalloc(1, (mwSize)sizeof(float));
                            *FloatValue_ptr = resp->Roi[i].AllFrames[j].CanvasY1;
                            dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
                            dims[1] = 1;
                            Data_ptr = mxCreateNumericArray(2, dims, mxSINGLE_CLASS, mxREAL);
                            mxSetM(Data_ptr, 1);
                            mxSetData(Data_ptr, FloatValue_ptr); 
                            //mxSetField(mxGetField(mxGetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i]), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[4], Data_ptr);
                            mxSetField(mxGetField(mxGetFieldByNumber(RoisStruct_ptr, 0, i), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[4], Data_ptr);
                            
                            FloatValue_ptr = (float *)mxCalloc(1, (mwSize)sizeof(float));
                            *FloatValue_ptr = resp->Roi[i].AllFrames[j].CanvasWidth;
                            dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
                            dims[1] = 1;
                            Data_ptr = mxCreateNumericArray(2, dims, mxSINGLE_CLASS, mxREAL);
                            mxSetM(Data_ptr, 1);
                            mxSetData(Data_ptr, FloatValue_ptr); 
                            //mxSetField(mxGetField(mxGetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i]), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[5], Data_ptr);
                            mxSetField(mxGetField(mxGetFieldByNumber(RoisStruct_ptr, 0, i), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[5], Data_ptr);
                            
                            FloatValue_ptr = (float *)mxCalloc(1, (mwSize)sizeof(float));
                            *FloatValue_ptr = resp->Roi[i].AllFrames[j].CanvasHeight;
                            dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
                            dims[1] = 1;
                            Data_ptr = mxCreateNumericArray(2, dims, mxSINGLE_CLASS, mxREAL);
                            mxSetM(Data_ptr, 1);
                            mxSetData(Data_ptr, FloatValue_ptr); 
                            //mxSetField(mxGetField(mxGetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i]), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[6], Data_ptr);
                            mxSetField(mxGetField(mxGetFieldByNumber(RoisStruct_ptr, 0, i), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[6], Data_ptr);
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
                        dims[0] = (mwSize)resp->Roi[i].FrameCount;
                        dims[1] = 1;
                        //mxSetField(mxGetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i]), 0, (const char *)RoiFieldNames_ptr[2], mxCreateStructArray(2, dims, 7, (const char **)FrameFieldNames_ptr));
                        mxSetField(mxGetFieldByNumber(RoisStruct_ptr, 0, i), 0, (const char *)RoiFieldNames_ptr[2], mxCreateStructArray(2, dims, 7, (const char **)FrameFieldNames_ptr));
                        for (j = 0; j < resp->Roi[i].FrameCount; j++) {
                            Int64Value_ptr = (int64 *)mxCalloc(1, (mwSize)sizeof(int64));
                            *Int64Value_ptr = (int64)(resp->Roi[i].AllFrames[j].TimeInSec * 1000.0);
                            dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
                            dims[1] = 1;
                            Data_ptr = mxCreateNumericArray(2, dims, mxINT64_CLASS, mxREAL);
                            mxSetM(Data_ptr, 1);
                            mxSetData(Data_ptr, Int64Value_ptr); 
                            //mxSetField(mxGetField(mxGetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i]), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[0], Data_ptr);
                            mxSetField(mxGetField(mxGetFieldByNumber(RoisStruct_ptr, 0, i), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[0], Data_ptr);
                            
                            FloatValue_ptr = (float *)mxCalloc(1, (mwSize)sizeof(float));
                            *FloatValue_ptr = resp->Roi[i].AllFrames[j].CanvasXm;
                            dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
                            dims[1] = 1;
                            Data_ptr = mxCreateNumericArray(2, dims, mxSINGLE_CLASS, mxREAL);
                            mxSetM(Data_ptr, 1);
                            mxSetData(Data_ptr, FloatValue_ptr); 
                            //mxSetField(mxGetField(mxGetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i]), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[1], Data_ptr);
                            mxSetField(mxGetField(mxGetFieldByNumber(RoisStruct_ptr, 0, i), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[1], Data_ptr);
                            
                            FloatValue_ptr = (float *)mxCalloc(1, (mwSize)sizeof(float));
                            *FloatValue_ptr = resp->Roi[i].AllFrames[j].CanvasYm;
                            dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
                            dims[1] = 1;
                            Data_ptr = mxCreateNumericArray(2, dims, mxSINGLE_CLASS, mxREAL);
                            mxSetM(Data_ptr, 1);
                            mxSetData(Data_ptr, FloatValue_ptr); 
                            //mxSetField(mxGetField(mxGetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i]), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[2], Data_ptr);
                            mxSetField(mxGetField(mxGetFieldByNumber(RoisStruct_ptr, 0, i), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[2], Data_ptr);
                            
                            FloatValue_ptr = (float *)mxCalloc(1, (mwSize)sizeof(float));
                            *FloatValue_ptr = resp->Roi[i].AllFrames[j].CanvasRadiusX;
                            dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
                            dims[1] = 1;
                            Data_ptr = mxCreateNumericArray(2, dims, mxSINGLE_CLASS, mxREAL);
                            mxSetM(Data_ptr, 1);
                            mxSetData(Data_ptr, FloatValue_ptr); 
                            //mxSetField(mxGetField(mxGetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i]), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[3], Data_ptr);
                            mxSetField(mxGetField(mxGetFieldByNumber(RoisStruct_ptr, 0, i), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[3], Data_ptr);
                            
                            FloatValue_ptr = (float *)mxCalloc(1, (mwSize)sizeof(float));
                            *FloatValue_ptr = resp->Roi[i].AllFrames[j].CanvasRadiusY;
                            dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
                            dims[1] = 1;
                            Data_ptr = mxCreateNumericArray(2, dims, mxSINGLE_CLASS, mxREAL);
                            mxSetM(Data_ptr, 1);
                            mxSetData(Data_ptr, FloatValue_ptr); 
                            //mxSetField(mxGetField(mxGetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i]), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[4], Data_ptr);
                            mxSetField(mxGetField(mxGetFieldByNumber(RoisStruct_ptr, 0, i), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[4], Data_ptr);
                            
                            FloatValue_ptr = (float *)mxCalloc(1, (mwSize)sizeof(float));
                            *FloatValue_ptr = resp->Roi[i].AllFrames[j].CanvasWidth;
                            dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
                            dims[1] = 1;
                            Data_ptr = mxCreateNumericArray(2, dims, mxSINGLE_CLASS, mxREAL);
                            mxSetM(Data_ptr, 1);
                            mxSetData(Data_ptr, FloatValue_ptr); 
                            //mxSetField(mxGetField(mxGetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i]), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[5], Data_ptr);
                            mxSetField(mxGetField(mxGetFieldByNumber(RoisStruct_ptr, 0, i), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[5], Data_ptr);
                            
                            FloatValue_ptr = (float *)mxCalloc(1, (mwSize)sizeof(float));
                            *FloatValue_ptr = resp->Roi[i].AllFrames[j].CanvasHeight;
                            dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
                            dims[1] = 1;
                            Data_ptr = mxCreateNumericArray(2, dims, mxSINGLE_CLASS, mxREAL);
                            mxSetM(Data_ptr, 1);
                            mxSetData(Data_ptr, FloatValue_ptr); 
                            //mxSetField(mxGetField(mxGetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i]), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[6], Data_ptr);
                            mxSetField(mxGetField(mxGetFieldByNumber(RoisStruct_ptr, 0, i), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[6], Data_ptr);
                        }
                        break;
                    case ShapeFree:
                        
                    case ShapePoints:
                        FrameFieldNames_ptr[0] = "MAPPS_time";
                        FrameFieldNames_ptr[1] = "Vertices";
                        FrameFieldNames_ptr[2] = "Width";
                        FrameFieldNames_ptr[3] = "Height";
                        dims[0] = (mwSize)resp->Roi[i].FrameCount;
                        dims[1] = 1;
                        //mxSetField(mxGetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i]), 0, (const char *)RoiFieldNames_ptr[2], mxCreateStructArray(2, dims, 5, (const char **)FrameFieldNames_ptr));
                        mxSetField(mxGetFieldByNumber(RoisStruct_ptr, 0, i), 0, (const char *)RoiFieldNames_ptr[2], mxCreateStructArray(2, dims, 5, (const char **)FrameFieldNames_ptr));
                        for (j = 0; j < resp->Roi[i].FrameCount; j++) {
                            Int64Value_ptr = (int64 *)mxCalloc(1, (mwSize)sizeof(int64));
                            *Int64Value_ptr = (int64)(resp->Roi[i].AllFrames[j].TimeInSec * 1000.0);
                            dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
                            dims[1] = 1;
                            Data_ptr = mxCreateNumericArray(2, dims, mxINT64_CLASS, mxREAL);
                            mxSetM(Data_ptr, 1);
                            mxSetData(Data_ptr, Int64Value_ptr); 
                            //mxSetField(mxGetField(mxGetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i]), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[0], Data_ptr);
                            mxSetField(mxGetField(mxGetFieldByNumber(RoisStruct_ptr, 0, i), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[0], Data_ptr);
                            /*
                            IntValue_ptr = (int *)mxCalloc(1, (mwSize)sizeof(int));
                            *IntValue_ptr = resp->Roi[i].AllFrames[j].VertexCount;
                            dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
                            dims[1] = 1;
                            Data_ptr = mxCreateNumericArray(2, dims, mxINT32_CLASS, mxREAL);
                            mxSetM(Data_ptr, 1);
                            mxSetData(Data_ptr, IntValue_ptr); 
                            //mxSetField(mxGetField(mxGetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i]), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[1], Data_ptr);
                            mxSetField(mxGetField(mxGetFieldByNumber(RoisStruct_ptr, 0, i), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[1], Data_ptr);
                            */
                            nV = resp->Roi[i].AllFrames[j].VertexCount;
                            FloatMatrix_ptr = (float *)mxCalloc((mwSize)nV * 2, (mwSize)sizeof(float));
                            FloatValue_ptr = FloatMatrix_ptr;
                            Vertex_ptr = resp->Roi[i].AllFrames[j].FreeVerticies;
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
                            *FloatValue_ptr = resp->Roi[i].AllFrames[j].CanvasWidth;
                            dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
                            dims[1] = 1;
                            Data_ptr = mxCreateNumericArray(2, dims, mxSINGLE_CLASS, mxREAL);
                            mxSetM(Data_ptr, 1);
                            mxSetData(Data_ptr, FloatValue_ptr); 
                            //mxSetField(mxGetField(mxGetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i]), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[2], Data_ptr);
                            mxSetField(mxGetField(mxGetFieldByNumber(RoisStruct_ptr, 0, i), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[2], Data_ptr);
                            
                            FloatValue_ptr = (float *)mxCalloc(1, (mwSize)sizeof(float));
                            *FloatValue_ptr = resp->Roi[i].AllFrames[j].CanvasHeight;
                            dims[0] = 0;    // if = 1, call mxFree(mxGetPr(Data_ptr)) before mxSetData(Data_ptr, TimeValue_ptr)
                            dims[1] = 1;
                            Data_ptr = mxCreateNumericArray(2, dims, mxSINGLE_CLASS, mxREAL);
                            mxSetM(Data_ptr, 1);
                            mxSetData(Data_ptr, FloatValue_ptr); 
                            //mxSetField(mxGetField(mxGetField(RoisStruct_ptr, 0, (const char *)RoiNames_ptr[i]), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[3], Data_ptr);
                            mxSetField(mxGetField(mxGetFieldByNumber(RoisStruct_ptr, 0, i), 0, RoiFieldNames_ptr[2]), (mwIndex)j, FrameFieldNames_ptr[3], Data_ptr);
                        }
                        break;
                    default:

                        break;
                }
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
