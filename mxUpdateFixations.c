/*=================================================================
 * mxUpdateFixations.c 
 *
 *=================================================================*/

#include "mxDataIO.h"
#include "ctype.h"

void Copy_Matlab_Vector_to_MAPPS_Vector(
    const uint64 nT, 
    const void *InVector, 
    const mxClassID Matlab_type, 
    void *OutVector, 
    const int MAPPS_type, 
    const size_t incrementInBytes,
    void *global, void *client, void *h1, void *h2,
    const char *Optional_Name_string)
{
    int showTypeWarning;
    uint64 j;
    
    unsigned char *UInt8_Matlab;
    int *Int32_Matlab;
    unsigned int *UInt32_Matlab;    
    int64 *Int64_Matlab;
    uint64 *UInt64_Matlab;
    float *Float_Matlab;
    double *Double_Matlab;
    
    unsigned char *UChar_MAPPS;
    int *Int_MAPPS;
    int64 *Int64_MAPPS;
    float *Float_MAPPS;
    double *Double_MAPPS;
    
    unsigned long raw_f = 0x7fc00000;
    unsigned long long raw_d = 0x7ff8000000000000;
    
    switch (MAPPS_type) {
        case MappsTypeInt:
            showTypeWarning = 1;
            if (incrementInBytes < sizeof(int)) {
                DisconnectFromClient(h2, (EDS_Client *)client);
                DisconnectFromGlobal(h1, (EDS_GlobalState *)global);
                mexPrintf("Increment is smaller than target size. \n");
                mexErrMsgTxt("Matlab ERROR!");
            }
            else if (incrementInBytes > sizeof(int)) {
                UChar_MAPPS = (char *)OutVector;
                switch (Matlab_type) {
                    case mxUINT8_CLASS:
                        UInt8_Matlab = (unsigned char *)InVector; 
                        for (j = 0; j < nT; j++) {
                            Int_MAPPS = (int *)UChar_MAPPS;
                            *Int_MAPPS = (int)*UInt8_Matlab++;
                            UChar_MAPPS += incrementInBytes;
                        }
                        break;
                    case mxINT32_CLASS:
                        Int32_Matlab = (int *)InVector; 
                        for (j = 0; j < nT; j++) {
                            Int_MAPPS = (int *)UChar_MAPPS;
                            *Int_MAPPS = *Int32_Matlab++;
                            UChar_MAPPS += incrementInBytes;
                        }
                        showTypeWarning = 0;
                        break;
                    case mxUINT32_CLASS:
                        UInt32_Matlab = (unsigned int *)InVector; 
                        for (j = 0; j < nT; j++) {
                            Int_MAPPS = (int *)UChar_MAPPS;
                            *Int_MAPPS = (int)*UInt32_Matlab++;
                            UChar_MAPPS += incrementInBytes;
                        }
                        break;
                    case mxINT64_CLASS:
                        Int64_Matlab = (int64 *)InVector; 
                        for (j = 0; j < nT; j++) {
                            Int_MAPPS = (int *)UChar_MAPPS;
                            *Int_MAPPS = (int)*Int64_Matlab++;
                            UChar_MAPPS += incrementInBytes;
                        }
                        break;
                    case mxUINT64_CLASS:
                        UInt64_Matlab = (uint64 *)InVector; 
                        for (j = 0; j < nT; j++) {
                            Int_MAPPS = (int *)UChar_MAPPS;
                            *Int_MAPPS = (int)*UInt64_Matlab++;
                            UChar_MAPPS += incrementInBytes;
                        }
                        break;
                    case mxSINGLE_CLASS:
                        Float_Matlab = (float *)InVector; 
                        for (j = 0; j < nT; j++) {
                            Int_MAPPS = (int *)UChar_MAPPS;
                            *Int_MAPPS = (int)*Float_Matlab++;
                            UChar_MAPPS += incrementInBytes;
                        }
                        break;
                    case mxDOUBLE_CLASS:
                        Double_Matlab = (double *)InVector; 
                        for (j = 0; j < nT; j++) {
                            Int_MAPPS = (int *)UChar_MAPPS;
                            *Int_MAPPS = (int)*Double_Matlab++;
                            UChar_MAPPS += incrementInBytes;
                        }
                        break;
                    default:
                        mexPrintf("Unhandled Matlab type for %s - passing zeros... \n", Optional_Name_string);
                        for (j = 0; j < nT; j++) {
                            Int_MAPPS = (int *)UChar_MAPPS;
                            *Int_MAPPS = 0;
                            UChar_MAPPS += incrementInBytes;
                        }
                        showTypeWarning = 0;
                        break;
                }
            }
            else {
                Int_MAPPS = (int *)OutVector;
                switch (Matlab_type) {
                    case mxUINT8_CLASS:
                        UInt8_Matlab = (unsigned char *)InVector; 
                        for (j = 0; j < nT; j++)
                            *Int_MAPPS++ = (int)*UInt8_Matlab++;
                        break;
                    case mxINT32_CLASS:
                        memcpy((void *)Int_MAPPS, (void *)InVector, (size_t)(nT) * sizeof(int));
                        showTypeWarning = 0;
                        break;
                    case mxUINT32_CLASS:
                        UInt32_Matlab = (unsigned int *)InVector; 
                        for (j = 0; j < nT; j++)
                            *Int_MAPPS++ = (int)*UInt32_Matlab++;
                        break;
                    case mxINT64_CLASS:
                        Int64_Matlab = (int64 *)InVector; 
                        for (j = 0; j < nT; j++)
                            *Int_MAPPS++ = (int)*Int64_Matlab++;
                        break;
                    case mxUINT64_CLASS:
                        UInt64_Matlab = (uint64 *)InVector; 
                        for (j = 0; j < nT; j++)
                            *Int_MAPPS++ = (int)*UInt64_Matlab++;
                        break;
                    case mxSINGLE_CLASS:
                        Float_Matlab = (float *)InVector; 
                        for (j = 0; j < nT; j++)
                            *Int_MAPPS++ = (int)*Float_Matlab++;
                        break;
                    case mxDOUBLE_CLASS:
                        Double_Matlab = (double *)InVector; 
                        for (j = 0; j < nT; j++)
                            *Int_MAPPS++ = (int)*Double_Matlab++;
                        break;
                    default:
                        mexPrintf("Unhandled Matlab type for %s - passing zeros... \n", Optional_Name_string);
                        for (j = 0; j < nT; j++)
                            *Int_MAPPS++ = 0;
                        showTypeWarning = 0;
                        break;
                }
            }
            if (showTypeWarning == 1)
                mexPrintf("Data transfer for %s might be faster if passed as 'int' from Matlab. \n", Optional_Name_string);
            break;
        case MappsTypeInt64:
            showTypeWarning = 1;
            if (incrementInBytes < sizeof(int64)) {
                DisconnectFromClient(h2, (EDS_Client *)client);
                DisconnectFromGlobal(h1, (EDS_GlobalState *)global);
                mexPrintf("Increment is smaller than target size. \n");
                mexErrMsgTxt("Matlab ERROR!");
            }
            else if (incrementInBytes > sizeof(int64)) {
                UChar_MAPPS = (char *)OutVector;
                switch (Matlab_type) {
                    case mxUINT8_CLASS:
                        UInt8_Matlab = (unsigned char *)InVector; 
                        for (j = 0; j < nT; j++) {
                            Int64_MAPPS = (int64 *)UChar_MAPPS;
                            *Int64_MAPPS = (int64)*UInt8_Matlab++;
                            UChar_MAPPS += incrementInBytes;
                        }
                        break;
                    case mxINT32_CLASS:
                        Int32_Matlab = (int *)InVector; 
                        for (j = 0; j < nT; j++) {
                            Int64_MAPPS = (int64 *)UChar_MAPPS;
                            *Int64_MAPPS = (int64)*Int32_Matlab++;
                            UChar_MAPPS += incrementInBytes;
                        }
                        break;
                    case mxUINT32_CLASS:
                        UInt32_Matlab = (unsigned int *)InVector; 
                        for (j = 0; j < nT; j++) {
                            Int64_MAPPS = (int64 *)UChar_MAPPS;
                            *Int64_MAPPS = (int64)*UInt32_Matlab++;
                            UChar_MAPPS += incrementInBytes;
                        }
                        break;
                    case mxINT64_CLASS:
                        Int64_Matlab = (int64 *)InVector; 
                        for (j = 0; j < nT; j++) {
                            Int64_MAPPS = (int64 *)UChar_MAPPS;
                            *Int64_MAPPS = *Int64_Matlab++;
                            UChar_MAPPS += incrementInBytes;
                        }
                        showTypeWarning = 0;
                        break;
                    case mxUINT64_CLASS:
                        UInt64_Matlab = (uint64 *)InVector; 
                        for (j = 0; j < nT; j++) {
                            Int64_MAPPS = (int64 *)UChar_MAPPS;
                            *Int64_MAPPS = (int64)*UInt64_Matlab++;
                            UChar_MAPPS += incrementInBytes;
                        }
                        break;
                    case mxSINGLE_CLASS:
                        Float_Matlab = (float *)InVector; 
                        for (j = 0; j < nT; j++) {
                            Int64_MAPPS = (int64 *)UChar_MAPPS;
                            *Int64_MAPPS = (int64)*Float_Matlab++;
                            UChar_MAPPS += incrementInBytes;
                        }
                        break;
                    case mxDOUBLE_CLASS:
                        Double_Matlab = (double *)InVector; 
                        for (j = 0; j < nT; j++) {
                            Int64_MAPPS = (int64 *)UChar_MAPPS;
                            *Int64_MAPPS = (int64)*Double_Matlab++;
                            UChar_MAPPS += incrementInBytes;
                        }
                        break;
                    default:
                        mexPrintf("Unhandled Matlab type for %s - passing zeros... \n", Optional_Name_string);
                        for (j = 0; j < nT; j++) {
                            Int64_MAPPS = (int64 *)UChar_MAPPS;
                            *Int64_MAPPS = 0;
                            UChar_MAPPS += incrementInBytes;
                        }
                        showTypeWarning = 0;
                        break;
                }
            }
            else {
                Int64_MAPPS = (int64 *)OutVector;
                switch (Matlab_type) {
                    case mxUINT8_CLASS:
                        UInt8_Matlab = (unsigned char *)InVector; 
                        for (j = 0; j < nT; j++)
                            *Int64_MAPPS++ = (int64)*UInt8_Matlab++;
                        break;
                    case mxINT32_CLASS:
                        Int32_Matlab = (int *)InVector; 
                        for (j = 0; j < nT; j++)
                            *Int64_MAPPS++ = (int64)*Int32_Matlab++;
                        break;
                    case mxUINT32_CLASS:
                        UInt32_Matlab = (unsigned int *)InVector; 
                        for (j = 0; j < nT; j++)
                            *Int64_MAPPS++ = (int64)*UInt32_Matlab++;
                        break;
                    case mxINT64_CLASS:
                        memcpy((void *)Int64_MAPPS, (void *)InVector, (size_t)(nT) * sizeof(int64));
                        showTypeWarning = 0;
                        break;
                    case mxUINT64_CLASS:
                        UInt64_Matlab = (uint64 *)InVector; 
                        for (j = 0; j < nT; j++)
                            *Int64_MAPPS++ = (int64)*UInt64_Matlab++;
                        break;
                    case mxSINGLE_CLASS:
                        Float_Matlab = (float *)InVector; 
                        for (j = 0; j < nT; j++)
                            *Int64_MAPPS++ = (int64)*Float_Matlab++;
                        break;
                    case mxDOUBLE_CLASS:
                        Double_Matlab = (double *)InVector; 
                        for (j = 0; j < nT; j++)
                            *Int64_MAPPS++ = (int64)*Double_Matlab++;
                        break;
                    default:
                        mexPrintf("Unhandled Matlab type for %s - passing zeros... \n", Optional_Name_string);
                        for (j = 0; j < nT; j++)
                            *Int64_MAPPS++ = 0;
                        showTypeWarning = 0;
                        break;
                }
            }
            if (showTypeWarning == 1)
                mexPrintf("Data transfer for %s might be faster if passed as 'int64' from Matlab. \n", Optional_Name_string);
            break;
        case MappsTypeFloat:
            showTypeWarning = 1;
            if (incrementInBytes < sizeof(float)) {
                DisconnectFromClient(h2, (EDS_Client *)client);
                DisconnectFromGlobal(h1, (EDS_GlobalState *)global);
                mexPrintf("Increment is smaller than target size. \n");
                mexErrMsgTxt("Matlab ERROR!");
            }
            else if (incrementInBytes > sizeof(float)) {
                UChar_MAPPS = (char *)OutVector;
                switch (Matlab_type) {
                    case mxUINT8_CLASS:
                        UInt8_Matlab = (unsigned char *)InVector; 
                        for (j = 0; j < nT; j++) {
                            Float_MAPPS = (float *)UChar_MAPPS;
                            *Float_MAPPS = (float)*UInt8_Matlab++;
                            UChar_MAPPS += incrementInBytes;
                        }
                        break;
                    case mxINT32_CLASS:
                        Int32_Matlab = (int *)InVector; 
                        for (j = 0; j < nT; j++) {
                            Float_MAPPS = (float *)UChar_MAPPS;
                            *Float_MAPPS = (float)*Int32_Matlab++;
                            UChar_MAPPS += incrementInBytes;
                        }
                        break;
                    case mxUINT32_CLASS:
                        UInt32_Matlab = (unsigned int *)InVector; 
                        for (j = 0; j < nT; j++) {
                            Float_MAPPS = (float *)UChar_MAPPS;
                            *Float_MAPPS = (float)*UInt32_Matlab++;
                            UChar_MAPPS += incrementInBytes;
                        }
                        break;
                    case mxINT64_CLASS:
                        Int64_Matlab = (int64 *)InVector; 
                        for (j = 0; j < nT; j++) {
                            Float_MAPPS = (float *)UChar_MAPPS;
                            *Float_MAPPS = (float)*Int64_Matlab++;
                            UChar_MAPPS += incrementInBytes;
                        }
                        break;
                    case mxUINT64_CLASS:
                        UInt64_Matlab = (uint64 *)InVector; 
                        for (j = 0; j < nT; j++) {
                            Float_MAPPS = (float *)UChar_MAPPS;
                            *Float_MAPPS = (float)*UInt64_Matlab++;
                            UChar_MAPPS += incrementInBytes;
                        }
                        break;
                    case mxSINGLE_CLASS:
                        Float_Matlab = (float *)InVector; 
                        for (j = 0; j < nT; j++) {
                            Float_MAPPS = (float *)UChar_MAPPS;
                            *Float_MAPPS = *Float_Matlab++;
                            UChar_MAPPS += incrementInBytes;
                        }
                        showTypeWarning = 0;
                        break;
                    case mxDOUBLE_CLASS:
                        Double_Matlab = (double *)InVector; 
                        for (j = 0; j < nT; j++) {
                            Float_MAPPS = (float *)UChar_MAPPS;
                            *Float_MAPPS = (float)*Double_Matlab++;
                            UChar_MAPPS += incrementInBytes;
                        }
                        break;
                    default:
                        mexPrintf("Unhandled Matlab type for %s - passing NaNs... \n", Optional_Name_string);
                        for (j = 0; j < nT; j++) {
                            Float_MAPPS = (float *)UChar_MAPPS;
                            *Float_MAPPS = *(float *)&raw_f;
                            UChar_MAPPS += incrementInBytes;
                        }
                        showTypeWarning = 0;
                        break;
                }
            }
            else {
                Float_MAPPS = (float *)OutVector;
                switch (Matlab_type) {
                    case mxUINT8_CLASS:
                        UInt8_Matlab = (unsigned char *)InVector; 
                        for (j = 0; j < nT; j++)
                            *Float_MAPPS++ = (float)*UInt8_Matlab++;
                        break;
                    case mxINT32_CLASS:
                        Int32_Matlab = (int *)InVector; 
                        for (j = 0; j < nT; j++)
                            *Float_MAPPS++ = (float)*Int32_Matlab++;
                        break;
                    case mxUINT32_CLASS:
                        UInt32_Matlab = (unsigned int *)InVector; 
                        for (j = 0; j < nT; j++)
                            *Float_MAPPS++ = (float)*UInt32_Matlab++;
                        break;
                    case mxINT64_CLASS:
                        Int64_Matlab = (int64 *)InVector; 
                        for (j = 0; j < nT; j++)
                            *Float_MAPPS++ = (float)*Int64_Matlab++;
                        break;
                    case mxUINT64_CLASS:
                        UInt64_Matlab = (uint64 *)InVector; 
                        for (j = 0; j < nT; j++)
                            *Float_MAPPS++ = (float)*UInt64_Matlab++;
                        break;
                    case mxSINGLE_CLASS:
                        memcpy((void *)Float_MAPPS, (void *)InVector, (size_t)(nT) * sizeof(float));
                        showTypeWarning = 0;
                        break;
                    case mxDOUBLE_CLASS:
                        Double_Matlab = (double *)InVector; 
                        for (j = 0; j < nT; j++)
                            *Float_MAPPS++ = (float)*Double_Matlab++;
                        break;
                    default:
                        mexPrintf("Unhandled Matlab type for %s - passing NaNs... \n", Optional_Name_string);
                        for (j = 0; j < nT; j++)
                            *Float_MAPPS++ = *(float *)&raw_f;
                        showTypeWarning = 0;
                        break;
                }
            }
            if (showTypeWarning == 1)
                mexPrintf("Data transfer for %s might be faster if passed as 'single' from Matlab. \n", Optional_Name_string);
            break;
        case MappsTypeDouble:
            showTypeWarning = 1;
            if (incrementInBytes < sizeof(double)) {
                DisconnectFromClient(h2, (EDS_Client *)client);
                DisconnectFromGlobal(h1, (EDS_GlobalState *)global);
                mexPrintf("Increment is smaller than target size. \n");
                mexErrMsgTxt("Matlab ERROR!");
            }
            else if (incrementInBytes > sizeof(double)) {
                UChar_MAPPS = (char *)OutVector;
                switch (Matlab_type) {
                    case mxUINT8_CLASS:
                        UInt8_Matlab = (unsigned char *)InVector; 
                        for (j = 0; j < nT; j++) {
                            Double_MAPPS = (double *)UChar_MAPPS;
                            *Double_MAPPS = (double)*UInt8_Matlab++;
                            UChar_MAPPS += incrementInBytes;
                        }
                        break;
                    case mxINT32_CLASS:
                        Int32_Matlab = (int *)InVector; 
                        for (j = 0; j < nT; j++) {
                            Double_MAPPS = (double *)UChar_MAPPS;
                            *Double_MAPPS = (double)*Int32_Matlab++;
                            UChar_MAPPS += incrementInBytes;
                        }
                        break;
                    case mxUINT32_CLASS:
                        UInt32_Matlab = (unsigned int *)InVector; 
                        for (j = 0; j < nT; j++) {
                            Double_MAPPS = (double *)UChar_MAPPS;
                            *Double_MAPPS = (double)*UInt32_Matlab++;
                            UChar_MAPPS += incrementInBytes;
                        }
                        break;
                    case mxINT64_CLASS:
                        Int64_Matlab = (int64 *)InVector; 
                        for (j = 0; j < nT; j++) {
                            Double_MAPPS = (double *)UChar_MAPPS;
                            *Double_MAPPS = (double)*Int64_Matlab++;
                            UChar_MAPPS += incrementInBytes;
                        }
                        break;
                    case mxUINT64_CLASS:
                        UInt64_Matlab = (uint64 *)InVector; 
                        for (j = 0; j < nT; j++) {
                            Double_MAPPS = (double *)UChar_MAPPS;
                            *Double_MAPPS = (double)*UInt64_Matlab++;
                            UChar_MAPPS += incrementInBytes;
                        }
                        break;
                    case mxSINGLE_CLASS:
                        Float_Matlab = (float *)InVector; 
                        for (j = 0; j < nT; j++) {
                            Double_MAPPS = (double *)UChar_MAPPS;
                            *Double_MAPPS = (double)*Float_Matlab++;
                            UChar_MAPPS += incrementInBytes;
                        }
                        break;
                    case mxDOUBLE_CLASS:
                        Double_Matlab = (double *)InVector; 
                        for (j = 0; j < nT; j++) {
                            Double_MAPPS = (double *)UChar_MAPPS;
                            *Double_MAPPS = *Double_Matlab++;
                            UChar_MAPPS += incrementInBytes;
                        }
                        showTypeWarning = 0;
                        break;
                    default:
                        mexPrintf("Unhandled Matlab type for %s - passing NaNs... \n", Optional_Name_string);
                        for (j = 0; j < nT; j++) {
                            Double_MAPPS = (double *)UChar_MAPPS;
                            *Double_MAPPS = *(double *)&raw_d;
                            UChar_MAPPS += incrementInBytes;
                        }
                        showTypeWarning = 0;
                        break;
                }
            }
            else {
                Double_MAPPS = (double *)OutVector;
                switch (Matlab_type) {
                    case mxUINT8_CLASS:
                        UInt8_Matlab = (unsigned char *)InVector; 
                        for (j = 0; j < nT; j++)
                            *Double_MAPPS++ = (double)*UInt8_Matlab++;
                        break;
                    case mxINT32_CLASS:
                        Int32_Matlab = (int *)InVector; 
                        for (j = 0; j < nT; j++)
                            *Double_MAPPS++ = (double)*Int32_Matlab++;
                        break;
                    case mxUINT32_CLASS:
                        UInt32_Matlab = (unsigned int *)InVector; 
                        for (j = 0; j < nT; j++)
                            *Double_MAPPS++ = (double)*UInt32_Matlab++;
                        break;
                    case mxINT64_CLASS:
                        Int64_Matlab = (int64 *)InVector; 
                        for (j = 0; j < nT; j++)
                            *Double_MAPPS++ = (double)*Int64_Matlab++;
                        break;
                    case mxUINT64_CLASS:
                        UInt64_Matlab = (uint64 *)InVector; 
                        for (j = 0; j < nT; j++)
                            *Double_MAPPS++ = (double)*UInt64_Matlab++;
                        break;
                    case mxSINGLE_CLASS:
                        Float_Matlab = (float *)InVector; 
                        for (j = 0; j < nT; j++)
                            *Double_MAPPS++ = (double)*Float_Matlab++;
                        break;
                    case mxDOUBLE_CLASS:
                        memcpy((void *)Double_MAPPS, (void *)InVector, (size_t)(nT) * sizeof(double));
                        showTypeWarning = 0;
                        break;
                    default:
                        mexPrintf("Unhandled Matlab type for %s - passing NaNs... \n", Optional_Name_string);
                        for (j = 0; j < nT; j++)
                            *Double_MAPPS++ = *(double *)&raw_d;
                        showTypeWarning = 0;
                        break;
                }
            }
            if (showTypeWarning == 1)
                mexPrintf("Data transfer for %s might be faster if passed as 'double' from Matlab. \n", Optional_Name_string);
            break;
        case MappsTypeUnknown:
            DisconnectFromClient(h2, (EDS_Client *)client);
            DisconnectFromGlobal(h1, (EDS_GlobalState *)global);
            mexPrintf("Type for %s reported as 'unknown' by server. \n", Optional_Name_string);
            mexErrMsgTxt("SERVER ERROR!");
            break;
        default:
            DisconnectFromClient(h2, (EDS_Client *)client);
            DisconnectFromGlobal(h1, (EDS_GlobalState *)global);
            mexPrintf("No or unrecognized type for %s as reported by server. \n", Optional_Name_string);
            mexErrMsgTxt("SERVER ERROR!");
            break;
    } 
}


void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
#include "_SetUpConnection_global_declaration.h"
#include "_SetUpConnection_client_declaration.h"
    
    size_t bufferSize;
    void *Buffer; 	
    DS_Fixations *req;
    
    const mxArray *FixationData_ptr, *CanvasIndex_ptr, *From_MAPPS_time_ptr, *To_MAPPS_time_ptr, *MidPointX_ptr, *MidPointY_ptr;
    const char *Field_string;
    const void *CanvasIndex, *From_MAPPS_time, *To_MAPPS_time, *MidPointX, *MidPointY;
    char *Test_string;
    mwSize dims[2];
    int nFields, fieldNumber_CanvasIndex, fieldNumber_From_MAPPS_time, fieldNumber_To_MAPPS_time, fieldNumber_MidPointX, fieldNumber_MidPointY, subjectIndex;
    uint64 nFixations, nFixations_temp;
    size_t Name_length_current, Name_length, incrementInBytes;
    DS_FixationEntry *Fix; 
    int i, j;

    
    //////////////////////////////////////////////////////////////////////////
	// 0. Initialize variables
	//////////////////////////////////////////////////////////////////////////

    Test_string = NULL;
    
    
    //////////////////////////////////////////////////////////////////////////
	// 1. Check for proper number of input and output arguments
	//////////////////////////////////////////////////////////////////////////
    
    /*  prhs[1] <-- subject index
     *  prhs[2] <-- struct containing fields for fixation data ("CanvasIndex", "From_MAPPS_time", "To_MAPPS_time", "MidPointX", "MidPointY") 
     
     *  plhs[0] <-- error code */
    
    if (!(nrhs == 3))
        mexErrMsgTxt("Input argument combination allowed is [struct, scalar, struct('CanvasIndex', 'From_MAPPS_time', 'To_MAPPS_time', 'MidPointX', 'MidPointY')]. \n");
    
    if (!(!(mxIsEmpty(prhs[0])) && mxIsStruct(prhs[0])))
        mexErrMsgTxt("First input argument is not a struct.\n");
    
    if (!(!(mxIsEmpty(prhs[1])) && mxIsNumeric(prhs[1]) && mxGetNumberOfDimensions(prhs[1]) == 2 && mxGetM(prhs[1]) == 1 && mxGetN(prhs[1]) == 1 && mxGetScalar(prhs[1]) > 0))
        mexErrMsgTxt("Second input argument is not a positive scalar.\n");
    
    if (!(!(mxIsEmpty(prhs[2])) && mxIsStruct(prhs[2]) && mxGetNumberOfFields(prhs[2]) >= 5))
        mexErrMsgTxt("Third input argument is not a struct consisting of at least 5 fields.\n");
       
    FixationData_ptr = prhs[2];
    nFields = mxGetNumberOfFields(FixationData_ptr);
    fieldNumber_CanvasIndex = -1;
    fieldNumber_From_MAPPS_time = -1;
    fieldNumber_To_MAPPS_time = -1;
    fieldNumber_MidPointX = -1;
    fieldNumber_MidPointY = -1;
    Name_length_current = 0;
    for (i = 0; i < nFields; i++) {
        Field_string = mxGetFieldNameByNumber(FixationData_ptr, i);
        Name_length = strlen(Field_string) + 1;
        if (Name_length > Name_length_current) {
            mxFree(Test_string);
            Name_length_current = Name_length;
            Test_string = (char *)mxCalloc((mwSize)Name_length_current, sizeof(char));
        }
        strcpy(Test_string, Field_string);
        for (j = 0; j < (int)Name_length - 1; j++)
            Test_string[j] = tolower(Test_string[j]);
              
        if (strcmp(Test_string, "Canvasindex") == 0) {
            if (fieldNumber_CanvasIndex < 0)
                fieldNumber_CanvasIndex = i;
            else
                mexErrMsgTxt("Duplicate lowercase field names for 'CanvasIndex'. \n");
            CanvasIndex_ptr = mxGetFieldByNumber(FixationData_ptr, 0, fieldNumber_CanvasIndex);
            if (!(mxIsEmpty(CanvasIndex_ptr)) && (!(mxIsNumeric(CanvasIndex_ptr)) || (mxGetM(CanvasIndex_ptr) != 1 && mxGetN(CanvasIndex_ptr) != 1)))
                mexErrMsgTxt("Field for 'CanvasIndex' values is not a numeric vector.\n");
        }
        else if (strcmp(Test_string, "from_mapps_time") == 0) {
            if (fieldNumber_From_MAPPS_time < 0)
                fieldNumber_From_MAPPS_time = i;
            else
                mexErrMsgTxt("Duplicate lowercase field names for 'From_MAPPS_time'. \n");
            From_MAPPS_time_ptr = mxGetFieldByNumber(FixationData_ptr, 0, fieldNumber_From_MAPPS_time);
            if (!(mxIsNumeric(From_MAPPS_time_ptr)) || (mxGetM(From_MAPPS_time_ptr) != 1 && mxGetN(From_MAPPS_time_ptr) != 1))
                mexErrMsgTxt("Field for 'From_MAPPS_time' values is not a non-empty numeric vector.\n");
        }
        else if (strcmp(Test_string, "to_mapps_time") == 0) {
            if (fieldNumber_To_MAPPS_time < 0)
                fieldNumber_To_MAPPS_time = i;
            else
                mexErrMsgTxt("Duplicate lowercase field names for 'To_MAPPS_time'. \n");
            To_MAPPS_time_ptr = mxGetFieldByNumber(FixationData_ptr, 0, fieldNumber_To_MAPPS_time);
            if (!(mxIsNumeric(To_MAPPS_time_ptr)) || (mxGetM(To_MAPPS_time_ptr) != 1 && mxGetN(To_MAPPS_time_ptr) != 1))
                mexErrMsgTxt("Field for 'To_MAPPS_time' values is not a non-empty numeric vector.\n");
        }
        else if (strcmp(Test_string, "midpointx") == 0) {
            if (fieldNumber_MidPointX < 0)
                fieldNumber_MidPointX = i;
            else
                mexErrMsgTxt("Duplicate lowercase field names for 'MidPointX'. \n");
            MidPointX_ptr = mxGetFieldByNumber(FixationData_ptr, 0, fieldNumber_MidPointX);
            if (!(mxIsNumeric(MidPointX_ptr)) || (mxGetM(MidPointX_ptr) != 1 && mxGetN(MidPointX_ptr) != 1))
                mexErrMsgTxt("Field for 'MidPointX' values is not a non-empty numeric vector.\n");
        }
        else if (strcmp(Test_string, "midpointy") == 0) {
            if (fieldNumber_MidPointY < 0)
                fieldNumber_MidPointY = i;
            else
                mexErrMsgTxt("Duplicate lowercase field names for 'MidPointY'. \n");
            MidPointY_ptr = mxGetFieldByNumber(FixationData_ptr, 0, fieldNumber_MidPointY);
            if (!(mxIsNumeric(MidPointY_ptr)) || (mxGetM(MidPointY_ptr) != 1 && mxGetN(MidPointY_ptr) != 1))
                mexErrMsgTxt("Field for 'MidPointY' values is not a non-empty numeric vector.\n");
        }
    }
    
    nFixations = 0;
    if (fieldNumber_CanvasIndex < 0)
        mexPrintf("'CanvasIndex' values will default to zero. \n");
    else {
        nFixations = (uint64)mxGetNumberOfElements(CanvasIndex_ptr);
        if (nFixations == 0)                                                // to handle empty 'CanvasIndex' field
            fieldNumber_CanvasIndex = -1;
    }
    if (fieldNumber_From_MAPPS_time < 0)
        mexErrMsgTxt("No values provided for 'From_MAPPS_time'");
    nFixations_temp = (uint64)mxGetNumberOfElements(From_MAPPS_time_ptr);
    if (nFixations > 0 && nFixations != nFixations_temp)
        mexErrMsgTxt("Inconsistent number of fixations between 'CanvasIndex' and 'From_MAPPS_time'");
    else
        nFixations = nFixations_temp;    
    if (fieldNumber_To_MAPPS_time < 0)
        mexErrMsgTxt("No values provided for 'To_MAPPS_time'");
    nFixations_temp = (uint64)mxGetNumberOfElements(To_MAPPS_time_ptr);
    if (nFixations != nFixations_temp)
        mexErrMsgTxt("Inconsistent number of fixations between ('CanvasIndex', 'From_MAPPS_time') and 'To_MAPPS_time'");
    if (fieldNumber_MidPointX < 0)
        mexErrMsgTxt("No values provided for 'MidPointX'");
    nFixations_temp = (uint64)mxGetNumberOfElements(MidPointX_ptr);
    if (nFixations != nFixations_temp)
        mexErrMsgTxt("Inconsistent number of fixations between ('CanvasIndex', 'From_MAPPS_time', 'To_MAPPS_time') and 'MidPointX'");
    if (fieldNumber_MidPointY < 0)
        mexErrMsgTxt("No values provided for 'MidPointY'");
    nFixations_temp = (uint64)mxGetNumberOfElements(MidPointY_ptr);
    if (nFixations != nFixations_temp)
        mexErrMsgTxt("Inconsistent number of fixations between ('CanvasIndex', 'From_MAPPS_time', 'To_MAPPS_time', 'MidPointX') and 'MidPointY'");    
              
    if (!(nlhs == 0 || nlhs == 1)){
        mexErrMsgTxt("No more than a single output argument is required.");
    }
    
    
    //////////////////////////////////////////////////////////////////////////
	// 2. Extract data passed from Matlab
	////////////////////////////////////////////////////////////////////////// 
    
    subjectIndex = (int)mxGetScalar(prhs[1]);
    
    if (fieldNumber_CanvasIndex > -1)
        CanvasIndex = (const void *)mxGetData(CanvasIndex_ptr);
    From_MAPPS_time = (const void *)mxGetData(From_MAPPS_time_ptr);
    To_MAPPS_time = (const void *)mxGetData(To_MAPPS_time_ptr);
    MidPointX = (const void *)mxGetData(MidPointX_ptr);
    MidPointY = (const void *)mxGetData(MidPointY_ptr);
    
    
    //////////////////////////////////////////////////////////////////////////
	// 2. Connect to the MAPPS shared memory buffer
	//////////////////////////////////////////////////////////////////////////    
    
#include "_SetUpConnection_global_code.h"
#include "_SetUpConnection_client_code.h"
    
    Buffer = ConnectToClient(&h2, client);
	
	if(Buffer == NULL)
	{
		DisconnectFromGlobal(h1, global);
		mexErrMsgIdAndTxt("MEX:UpdateFixations:UnableToConnectClient", "Unable to connect to shared memory buffer (client state)! \n");
	}
		
		
	//////////////////////////////////////////////////////////////////////////
	// 3. Prepare the buffer for use
	//////////////////////////////////////////////////////////////////////////
	
	if(Prepare(global, client, EDS_SetFixations) == FALSE)
	{
		DisconnectFromClient(h2, client);
        DisconnectFromGlobal(h1, global);
		mexErrMsgIdAndTxt("MEX:UpdateFixations:ClientBlocked", "Server is still blocking the client channel! \n");
	}


    //////////////////////////////////////////////////////////////////////////
	// 4. Populate
	//////////////////////////////////////////////////////////////////////////

	req = (DS_Fixations *)Buffer;
    bufferSize = 0;
    
    req->SubjectIndex = (int)subjectIndex - 1;  // 0-based for MAPPS
	req->Count = (int)nFixations;
    
    bufferSize += sizeof(DS_Fixations);

	Fix = (DS_FixationEntry *)&(req->Entry);
    incrementInBytes = sizeof(DS_FixationEntry);
    if (fieldNumber_CanvasIndex > -1) 
        Copy_Matlab_Vector_to_MAPPS_Vector((const uint64)nFixations, 
                                           CanvasIndex, 
                                           (const mxClassID)mxGetClassID(CanvasIndex_ptr), 
                                           (void *)&(Fix[0].DisplayIndex), 
                                           MappsTypeInt, 
                                           (const size_t)incrementInBytes,
                                           (void *)global, (void *)client, h1, h2,
                                           "'CanvasIndex'");
    else
        Copy_Matlab_Vector_to_MAPPS_Vector((const uint64)nFixations, 
                                           NULL,                                            // setting to pass zeros
                                           mxUNKNOWN_CLASS,                                 // setting to pass zeros
                                           (void *)&(Fix[0].DisplayIndex), 
                                           MappsTypeInt, 
                                           (const size_t)incrementInBytes,
                                           (void *)global, (void *)client, h1, h2,
                                           "'CanvasIndex'");
    Copy_Matlab_Vector_to_MAPPS_Vector((const uint64)nFixations, 
                                       From_MAPPS_time, 
                                       (const mxClassID)mxGetClassID(From_MAPPS_time_ptr), 
                                       (void *)&(Fix[0].TimeStartMilli), 
                                       MappsTypeInt64,                                      
                                       (const size_t)incrementInBytes,
                                       (void *)global, (void *)client, h1, h2,
                                       "'From_MAPPS_time'");
    Copy_Matlab_Vector_to_MAPPS_Vector((const uint64)nFixations, 
                                       To_MAPPS_time, 
                                       (const mxClassID)mxGetClassID(To_MAPPS_time_ptr), 
                                       (void *)&(Fix[0].TimeEndMilli), 
                                       MappsTypeInt64,                                        
                                       (const size_t)incrementInBytes,
                                       (void *)global, (void *)client, h1, h2,
                                       "'To_MAPPS_time'");
    Copy_Matlab_Vector_to_MAPPS_Vector((const uint64)nFixations, 
                                       MidPointX, 
                                       (const mxClassID)mxGetClassID(MidPointX_ptr), 
                                       (void *)&(Fix[0].MidPointX), 
                                       MappsTypeFloat,
                                       (const size_t)incrementInBytes,
                                       (void *)global, (void *)client, h1, h2,
                                       "'MidPointX'");
    Copy_Matlab_Vector_to_MAPPS_Vector((const uint64)nFixations, 
                                       MidPointY, 
                                       (const mxClassID)mxGetClassID(MidPointY_ptr), 
                                       (void *)&(Fix[0].MidPointY), 
                                       MappsTypeFloat,
                                       (const size_t)incrementInBytes,
                                       (void *)global, (void *)client, h1, h2,
                                       "'MidPointY'");
    
    bufferSize += (size_t)nFixations * sizeof(DS_FixationEntry);
   
    
	//////////////////////////////////////////////////////////////////////////
	// 5. Execute
	//////////////////////////////////////////////////////////////////////////
	
	if(Execute(global, client, (int)bufferSize) == FALSE)
    {
        DisconnectFromClient(h2, client);
        DisconnectFromGlobal(h1, global);
        mexErrMsgIdAndTxt("MEX:UpdateFixations:ServerTimedOut", "SERVER TIMED OUT! \n");
    }
    
    
	//////////////////////////////////////////////////////////////////////////
	// 6. Echo the result
	//////////////////////////////////////////////////////////////////////////
    
    if(strncmp(client->Result, "success", 7) != 0 && nlhs == 0)
        mexPrintf("Details (mxUpdateFixations): %s\n", client->Details);
    
    
    //////////////////////////////////////////////////////////////////////////
	// 7. Populate Matlab output
	//////////////////////////////////////////////////////////////////////////
    
    if (nlhs == 1) {
        dims[0] = 0;
        dims[1] = 1;
        plhs[0] = mxCreateNumericArray(2, dims, mxDOUBLE_CLASS, mxREAL);
    }
    
	
	//////////////////////////////////////////////////////////////////////////
	// 8. Close shared memory handles
	//////////////////////////////////////////////////////////////////////////

    DisconnectFromClient(h2, client);
    DisconnectFromGlobal(h1, global);
    
    
    //////////////////////////////////////////////////////////////////////////
	// 9. Clean up allocated memory
	//////////////////////////////////////////////////////////////////////////
    
    mxFree(Test_string);
}
