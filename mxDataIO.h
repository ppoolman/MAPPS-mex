#include "mex.h"
#include "matrix.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define EDS_MAX_CLIENT_COUNT	32
#define EDS_GLOBAL_BUFFER_SIZE	100000
#define EDS_VERSION				20120525

#ifndef int16
#define int16 signed short
#endif

#ifndef uint16
#define uint16 unsigned short
#endif

#ifndef uint
#define uint unsigned int
#endif

#ifdef _WIN32
#define int64 __int64
#define uint64 unsigned long long
#else
#define int64 int64_t
#define uint64 uint64_t
#endif

#define MappsTypeUnknown	0
#define MappsTypeInt16		0x10002
#define MappsTypeInt		0x10004
#define MappsTypeInt64		0x10008
#define MappsTypeFloat		0x20004
#define MappsTypeDouble		0x20008
#define MappsTypeString		0x30000
#define MappsTypeBlob	    0x40000
#define MappsTypeVariable	0x50000

typedef enum
{
	EDS_NotSet,

	EDS_GetMessage		= 1000,		// Gets a pending message
	EDS_DeleteClient,				// Delete the current client

	EDS_SetTime			= 2000,		// Set the project time
	EDS_GetProjectName,				// Get the current project name

	EDS_CreateBus		= 3000,		// Creates a new bus (or replaces an existing one)
	EDS_DeleteBus,					// Deletes an entire bus
	EDS_ListBuses,					// Lists project buses
	EDS_RetrieveBuses,				// Retrieve contents for buses
	EDS_UpdateBus,					// Update the contents of a bus
	EDS_DeleteFrames,				// Deletes content from within a bus

	EDS_GetFixations	= 4000,		// Gets a list of fixations
	EDS_SetFixations,				// Sets a list of fixations
	EDS_UpdateFixations,			// Prompts an update for the fixations

	EDS_SetVisibility	= 5000,		// Sets signal visibility
	EDS_OverlaySignals,				// Overlays a set of signals

	EDS_GetCanvasSizes	= 6000,		// Gets the sizes of the canvas
	EDS_SetCanvas,					// Sets canvas content

	EDS_GetRoi			= 7000,		// Gets a list of ROI
	EDS_GetRoiIntersections,		// Compute ROI intersections
	EDS_GetRoiState,				// Get the current ROI state (count, last updated)
	EDS_InterpolateRoi,			// Interpolates ROI's at a specified time

	EDS_GetOptoRoi		= 8000,		// Gets a list of Opto ROI
	EDS_GetOptoIntersections,		// Computes Opto ROI intersections

	EDS_TEMP_UpdateTriggers = 9000,	// Send an opto trigger
	EDS_OptoSupMessage,				// Send a message on the supervisor channel
	EDS_OptoClientMessage,			// Send a message on the client channel

	DUMMY_TAIL
	
}RequestTypes;

typedef union
{
	void* AsPointer;
	int64 AsFlat;
	
} VoidPointer;

typedef enum 
{
	RunModeUnknown,		// Mode is not set
	RunModeStatic,		// Static file playback
	RunModeLive			// Live playback
	
} RunModes;

typedef struct
{
	//////////////////////////////////////////////////////////////////////////
	// STATIC STATE: Fields are set at initialization, then remain static.
	//////////////////////////////////////////////////////////////////////////

	int				Version;			// Version that the client has built against
	int				Index;				// Index in global state

	char			Name[128];			// Client's name
	char			Attributes[128];	// Client attributes
	char			Type[32];			// Client's type
	int				ChallengeResponse;	// Challenge response code
	float			KillOnIdleSec;		// Kill process if it has been idle for duration (0 for infinite)

	char			ShmemName[128];		// Name of the client shared memory
	int				ShmemSize;			// Size of shared memory, bytes
	VoidPointer		BufferPointer;		// Pointer to buffer pointer


	//////////////////////////////////////////////////////////////////////////
	// LOCKS: Access locks.
	//////////////////////////////////////////////////////////////////////////

	int				ClientLock;			// Client has locked the buffer
	int				ServerLock;			// Server has locked the buffer


	//////////////////////////////////////////////////////////////////////////
	// DYNAMIC STATE: Fields that are updated throughout the lifetime of the 
	//                client connection.
	//////////////////////////////////////////////////////////////////////////

	int				RequestIsLoaded;	// A request has been loaded by the client
	int				ServerIsProcessing;	// The server is currently processing client request
	int				RequestIsReady;		// The server has completed a request from the client

	RequestTypes	Request;			// Request to perform
	int				RequestSize;		// Size of the payload, in bytes. '-1' indicates full buffer
	char			Result[16];			// Result of the operation ('success', 'fail')
	char			Details[128];		// Details about the operation (use specific)

	int				MessageCount;		// Count of events waiting for client
	int				HeartBeat;			// Incremental counter

	char			Status[128];		// Status feedback from client to MAPPS, shown as pop-up balloon
	char			LabelText[128];		// Text to be shown in the SigPage client area

	
	//////////////////////////////////////////////////////////////////////////
	// PRIVATE: Server's private state. Do not alter.
	//////////////////////////////////////////////////////////////////////////

	VoidPointer		PrivateState;		// Private state, no not alter

} EDS_Client;

typedef struct
{
	int			Version;			// Software version
	int			HeartBeat;			// Generic heart beat indicator
	int64		Clock;				// System clock, UTC
	float		CurrentTime;		// Current project time, seconds
	int64		Epoch;				// Project epoch, UTC time
	int			ChallengeCode;		// Challenge request code
	RunModes	RunMode;			// Project run mode

	int			NewClient;			// Thrown when a client joins
	int			NewRequest;			// Throw when a client has posted a request

	EDS_Client	Clients[EDS_MAX_CLIENT_COUNT];

} EDS_GlobalState;


/////////////////////////////////////////////////////////////////////////
// Region of Interest types
//////////////////////////////////////////////////////////////////////////

#define MaxFreeVerticies 200
#define MaxFrameCount 100

#define ShapeUnknown 0
#define ShapeRectangle 1
#define ShapeEllipse 2
#define ShapeFree 3
#define ShapePoints 4

typedef struct
{
	int		Count;
	int64	LastUpdateTime;
	int		Frame;

} DS_RoiState;

typedef struct
{
	int64	TimeUtc;			// UTC timestamp
	float	TimeInSec;			// Time from epoch, in seconds
		
	float	CanvasX0;			// Canvas extents left
	float	CanvasX1;			// Canvas extents right
	float	CanvasY0;			// Canvas extents bottom
	float	CanvasY1;			// Canvas extents top

	float	CanvasXm;			// Canvas horizontal midpoint
	float	CanvasYm;			// Canvas vertical midpoint

	float	CanvasWidth;		// Canvas extents width
	float	CanvasHeight;		// Canvas extents height

	float	CanvasRadiusX;		// Canvas extents half width
	float	CanvasRadiusY;		// Canvas extents half height

	int		VertexCount;
	float	FreeVerticies[MaxFreeVerticies*2];

} DS_RoiFrame;

bool AnyArea(DS_RoiFrame* frame) { return frame->CanvasWidth>0 && frame->CanvasHeight>0; }

typedef struct
{
	int				InputRoiIndex;          // INPUT: Index of the ROI to interpolate
	int64			InputTimeMillis[256];   // INPUT: Time at which to interpolate

	char			Name[64];               // Name of the ROI
	int				Shape;                  // Shape of the ROI
	int				ScreenIndex;            // Screen index of the ROI
	int				ObjectIndex;            // Index of the object, or '-1' if none
	float			ObjectOrigin[3];        // Origin of the underlying object
	float			ObjectXVector[3];       // Left vector for underlying object
	float			ObjectYVector[3];       // Up vector for underlying object

	DS_RoiFrame		Frames[1];

} DS_InterpolatedFrame;

typedef struct
{
	char				Name[64];
	int					Shape;
	int					FrameCount;
	int					ScreenIndex;
	int					RepeatCount;
	DS_RoiFrame			AllFrames[MaxFrameCount];

} DS_RegionOfInterest;

typedef struct  
{
	int					Count;
	DS_RegionOfInterest	Roi[1];

} DS_RegionsOfInterest;

typedef struct
{
	int64	Time;
	int		Index;
	char	Name[64];

} DS_RoiIntersection;

typedef struct
{
	int64	Time;
	int		Index;
	float	IntersectionXyz[3];
	char	Name[64];

} DS_3dRoiIntersection;

typedef struct
{
	int	ComputeAs3D;
	int		SubjectIndex;

	int64	StartTime;
	int64	EndTime;

	int		FrameCount;
	
	union
	{
		DS_RoiIntersection		Intersections[1];
		DS_3dRoiIntersection	Intersections3d[1];
	};

} DS_RoiIntersections;


/////////////////////////////////////////////////////////////////////////
// OPTO ROI Types
//////////////////////////////////////////////////////////////////////////

# define MaxRollingFrameCount 500

typedef struct
{
	int64	Time;
	float	X, Y, W, H;
} DS_RollingRoiFrame;

typedef struct
{
	char			Name[64];
	int				FrameCount;

	DS_RollingRoiFrame	Frames[MaxRollingFrameCount];
	DS_RollingRoiFrame	InterpolatedFrame;
	
} DS_RollingRoi;

typedef struct
{
	int					Count;
	DS_RollingRoi	Roi[1];
	
} DS_RollingRegionsOfInterest;


/////////////////////////////////////////////////////////////////////////
// Get project name
//////////////////////////////////////////////////////////////////////////

typedef struct
{
	char	Name[512];

} DS_GetProjectNameResponse;


/////////////////////////////////////////////////////////////////////////
// Set time
//////////////////////////////////////////////////////////////////////////

typedef struct
{
	float	Time;
	char	Mode;

} DS_SetTimeRequest;


//////////////////////////////////////////////////////////////////////////
// Custom canvas
//////////////////////////////////////////////////////////////////////////

typedef struct
{
	char	Name[128];
	int		Width;
	int		Height;

} DS_CanvasSize;

typedef struct
{
	int				CanvasCount;
	DS_CanvasSize	Canvases[16];

} DS_CanvasSizesResponse;

typedef struct  
{
	char	CanvasName[128];
	int		Width;
	int		Height;
	char	Mode[8];
	char	Payload;

} DS_SetCanvas;


//////////////////////////////////////////////////////////////////////////
// Delete Frames - Deletes frames of a bus
//////////////////////////////////////////////////////////////////////////

typedef struct  
{
	char	BusName[128];
	int		DeleteByIndex;

	int		StartIndex;
	int		LastIndex;

	int64	StartTime;
	int64	EndTime;

} DS_DeleteFramesRequest;

//////////////////////////////////////////////////////////////////////////
// Delete Bus - Deletes an existing dynamic bus
//////////////////////////////////////////////////////////////////////////

typedef struct
{
	char	BusName[128];

} DS_DeleteBusRequest;


//////////////////////////////////////////////////////////////////////////
// Retrieve Data - Pull current data
//////////////////////////////////////////////////////////////////////////

typedef struct
{
	char	BusName[128];
	char	ElementNames[4096];
	int64	StartTimeMilli;
	int64	EndTimeMilli;
	int		IncludeIndicies;

} DS_RetrieveDataRequest;


//////////////////////////////////////////////////////////////////////////
// Bus Attributes - ???
//////////////////////////////////////////////////////////////////////////

typedef struct
{
	char	BusName[128];
	int64	StartTimeMilli;
	int64	EndTimeMilli;

} DS_BusAttributesRequest;


//////////////////////////////////////////////////////////////////////////
// List Bus - List all currently available buses
//////////////////////////////////////////////////////////////////////////

typedef struct
{
	char	ElementName[128];
	int		Type;
	int		SizeInBytes;

} DS_ListElementEntry;


//////////////////////////////////////////////////////////////////////////
// Update bus - Updates content of existing bus
//////////////////////////////////////////////////////////////////////////

typedef struct
{
	char				BusName[128];
	char				Mode[16];
	int					Count;

	int					ElementCount;
	DS_ListElementEntry	Elements[128];
	
	char				Payload;

} DS_UpdateBusRequest;


//////////////////////////////////////////////////////////////////////////
// Fixations
//////////////////////////////////////////////////////////////////////////

typedef struct
{
	int     SubjectIndex;   //0-base
	int64	StartTimeMilli;
	int64	EndTimeMilli;    // Set StartTimeMilli=EndTimeMilli=0 to return ALL data

} DS_RetrieveFixationRequest;

typedef struct
{
	int			DisplayIndex;

	float		MidPointX;
	float		MidPointY;

	int64         TimeStartMilli;
	int64         TimeEndMilli;

} DS_FixationEntry;

typedef struct
{
	int                 SubjectIndex;							
	int                 Count;
       
	DS_FixationEntry	Entry[1];

} DS_Fixations;

//////////////////////////////////////////////////////////////////////////
// List Bus - List all currently available buses
//////////////////////////////////////////////////////////////////////////

typedef struct
{
	char				BusName[128];
	int					StartTimeMs;
	int					EndTimeMs;
	int					FrameCount;
	int					ElementCount;
	DS_ListElementEntry	Elements[128];

} DS_ListBusEntry;

typedef struct
{
	int					GlobalStartTimeMs;
	int					GlobalEndTimeMs;
	int					BusCount;
	DS_ListBusEntry		Buses[128];

} DS_ListBusResponse;

typedef struct
{
    int        FrameCount;
    int        ElementCount;

    char       ElementNames[256][256];

    int        IsVariableEncoded;
    int        Types[128];
    int        SizeInBytes[128];
    int        LoadedSuccessfully[128];

    int        PointerToTimeStamps;        // Offset to beginning of timestamp array
    int        PointerToFrameSizes[128];   // Offset to beginning of frame size arrays (one per element)
    int        StartOfData[128];            // Offset to beginning of frame data

    char    StartOfPayload;  // All pointers are relative offsets from &StartOfPayload

} DS_RetrieveDataResponse;


//////////////////////////////////////////////////////////////////////////
// Create bus - Creates a new bus
//////////////////////////////////////////////////////////////////////////

typedef struct
{
	int					FrameCount;
	char				BusName[128];
	int					ElementCount;
	DS_ListElementEntry	Elements[128];

} DS_CreateBusRequest;

//////////////////////////////////////////////////////////////////////////
// Visibility attributes - Changes the visibility of a signal
//////////////////////////////////////////////////////////////////////////

typedef struct  
{
	char BusName[128];
	char ElementName[128];
	int  SignalIsVisible;

} DS_VisibilityItem;

typedef struct  
{
	int					Count;
	DS_VisibilityItem	Items[128];

} DS_VisibilityRequest;


//////////////////////////////////////////////////////////////////////////
// Overlay requests - Changes signal overlays
//////////////////////////////////////////////////////////////////////////

typedef struct
{
	char	PrimaryBusName[128];
	char	PrimaryElementName[128];

	char	SecondBusName[128];
	char	SecondElementName[128];

} DS_OverlayEntry;

typedef struct  
{
	int EntryCount;
	DS_OverlayEntry Entries[1];

} DS_OVerlayEntries;


//////////////////////////////////////////////////////////////////////////
// ConnectToGlobal()
// 
// Connects to the global buffer via a shared memory interface.
//
// Arguments:  h1 - Handle to store shared memory
// 
// Returns:    Pointer to the global buffer. May be NULL if an error was
//             encountered.
//////////////////////////////////////////////////////////////////////////

EDS_GlobalState* ConnectToGlobal(void** h1)
{
	EDS_GlobalState* global = NULL;
	
	SECURITY_ATTRIBUTES secAttr;
	char secDesc[SECURITY_DESCRIPTOR_MIN_LENGTH];
	
	secAttr.nLength = sizeof(secAttr);
	secAttr.bInheritHandle = FALSE;
	secAttr.lpSecurityDescriptor = &secDesc;
	InitializeSecurityDescriptor(secAttr.lpSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);
	SetSecurityDescriptorDacl(secAttr.lpSecurityDescriptor, TRUE, 0, FALSE);
    
	*h1 = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		&secAttr,
		PAGE_READWRITE,
		0, EDS_GLOBAL_BUFFER_SIZE, 
		"Local\\mapps.data_server.global");
		
	if(*h1 != NULL)
	{
		global = (EDS_GlobalState*)MapViewOfFile(*h1, FILE_MAP_ALL_ACCESS, 0, 0, 0);
		
		if(global == NULL)
		{
			return NULL;
		}
		else
		{
			if(global->Version == 0)
				mexPrintf("WARNING: No header was found \n");
			else if(global->Version != EDS_VERSION)
				mexPrintf("WARNING: Header version mis-match (%d != %d) \n", global->Version, EDS_VERSION);
				
			return global;
		}
	}
	else
	{
		mexPrintf("Unable to connect to shared memory buffer! \n");
		return NULL;
	}
}


//////////////////////////////////////////////////////////////////////////
// DisconnectFromGlobal()
// 
// Disconnects from the global buffer.
//
// Arguments:  h1 - Handle to shared memory (from connect call)
//             state - Pointer to state (from connect call)
// 
// Returns:    (void)
//////////////////////////////////////////////////////////////////////////

void DisconnectFromGlobal(void* h1, EDS_GlobalState* state)
{
	UnmapViewOfFile(state);
    CloseHandle(h1);
}


//////////////////////////////////////////////////////////////////////////
// RegisterClient()
// 
// Registers the client with MAPPS.
//
// Arguments:  details - Attributes that will be used to describe client
//             global - Pointer to state (from connect call)
// 
// Returns:    Pointer to the client. May be NULL if an error was
//             encountered.
//////////////////////////////////////////////////////////////////////////

EDS_Client* RegisterClient(EDS_Client* details, EDS_GlobalState* global)
{
	int i;
	
	details->PrivateState.AsPointer = NULL;
	details->BufferPointer.AsPointer = NULL;
	details->ClientLock = 0;
	details->ServerLock = 0;
	details->RequestIsLoaded = 0;
	details->ServerIsProcessing = 0;
	details->RequestIsReady = 0;
	details->Version = EDS_VERSION;
	
	for(i=0; i<EDS_MAX_CLIENT_COUNT; i++)
	{
		if(global->Clients[i].Name[0] == 0)
		{
			memcpy(&global->Clients[i], details, sizeof(EDS_Client));
			global->Clients[i].Index = i;
			global->NewClient++;
			
			while(global->NewClient != 0)
				Sleep(20);
			
		  return &global->Clients[i];
		}
	}
	
	return NULL;
}

//////////////////////////////////////////////////////////////////////////
// FindClientByIndex()
// 
// Searches for a client with a given index.
//
// Arguments:  index - Index to search for
//             global - Pointer to state (from connect call)
// 
// Returns:    Pointer to the client. May be NULL if not client is found
//////////////////////////////////////////////////////////////////////////

EDS_Client* FindClientByIndex(int index, EDS_GlobalState* global)
{
	if(global->Clients[index].Name[0] == 0)
		return NULL;
	else
		return &global->Clients[index];
}

//////////////////////////////////////////////////////////////////////////
// FindClientByName()
// 
// Searches for a client with a given name.
//
// Arguments:  index - Name to search for
//             global - Pointer to state (from connect call)
// 
// Returns:    Pointer to the client. May be NULL if not client is found
//////////////////////////////////////////////////////////////////////////

EDS_Client* FindClientByName(const char* name, EDS_GlobalState* global)
{
	int i;
	
	for(i=0; i<EDS_MAX_CLIENT_COUNT; i++)
	{
		if(strcmp(global->Clients[i].Name, name) == 0)
		{
		  return &global->Clients[i];
		}
	}
	
	return NULL;
}

//////////////////////////////////////////////////////////////////////////
// ConnectToClient()
// 
// Connects to the client buffer via a shared memory interface.
//
// Arguments:  h1 - Handle to store shared memory
//             client - Pointer to client details
// 
// Returns:    Pointer to the client buffer. May be NULL if an error was
//             encountered.
//////////////////////////////////////////////////////////////////////////

void* ConnectToClient(void** h2, EDS_Client* client)
{
	SECURITY_ATTRIBUTES secAttr;
	char secDesc[SECURITY_DESCRIPTOR_MIN_LENGTH];
	
	secAttr.nLength = sizeof(secAttr);
	secAttr.bInheritHandle = FALSE;
	secAttr.lpSecurityDescriptor = &secDesc;
	InitializeSecurityDescriptor(secAttr.lpSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);
	SetSecurityDescriptorDacl(secAttr.lpSecurityDescriptor, TRUE, 0, FALSE);
    
	*h2 = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		&secAttr,
		PAGE_READWRITE,
		0, client->ShmemSize, 
		client->ShmemName);
		
	if(*h2 != NULL)
	{
		client->BufferPointer.AsPointer = MapViewOfFile(*h2, FILE_MAP_ALL_ACCESS, 0, 0, 0);
		return client->BufferPointer.AsPointer;
	}
	else
	{
		return NULL;
	}
}

//////////////////////////////////////////////////////////////////////////
// DisconnectFromClient()
// 
// Disconnects from the client buffer.
//
// Arguments:  h2 - Handle to shared memory (from connect call)
//             client - Pointer to the client (from connect call)
// 
// Returns:    (void)
//////////////////////////////////////////////////////////////////////////

void DisconnectFromClient(void* h2, EDS_Client* client)
{
	BOOL retval;
	DWORD lastError = 0;
	
	if(client == NULL)
	{
		printf("Attempting to close a NULL client \n");
		return;
	}
	
	retval = UnmapViewOfFile(client->BufferPointer.AsPointer);
	
	if(retval == 0)
	{
		lastError = GetLastError();
		printf("DisconnectFromClient(): Error unmapping view, code=%d \n", lastError);
	}
	
    retval = CloseHandle(h2);
    
    if(retval == 0)
		{
			lastError = GetLastError();
			printf("DisconnectFromClient(): Error closing handle, code=%d \n", lastError);
		}
}

//////////////////////////////////////////////////////////////////////////
// Prepare()
// 
// Prepare a buffer for populating
//
// Arguments:  global - Global state (from connect call)
//             client - Client state (from connect call)
//             req - Type of request to submit
// 
// Returns:    TRUE if success, otherwise FALSE
//////////////////////////////////////////////////////////////////////////

BOOL Prepare(EDS_GlobalState* global, EDS_Client* client, RequestTypes req)
{
	if(client->ServerLock)
	{
		return FALSE;
	}
	
	client->ClientLock = 1;
	client->Request = req;
	client->RequestSize = -1;
	
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
// Execute()
// 
// Executes a pending command
//
// Arguments:  global - Global state (from connect call)
//             client - Client state (from connect call)
// 
// Returns:    TRUE if success, otherwise FALSE
//////////////////////////////////////////////////////////////////////////

BOOL Execute(EDS_GlobalState* global, EDS_Client* client, int size)
{
	int counter = 0;
	
	client->ClientLock = 0;
	client->RequestIsReady = 0;
	client->ServerIsProcessing = 0;
	client->RequestIsLoaded = 1;
    client->RequestSize = size;
    //mexPrintf("Size of buffer = %d bytes \n", client->RequestSize);
	
	global->NewRequest++;
	
	while(client->RequestIsReady == 0)
	{
		if(++counter%50 == 0) {
			mexPrintf("[%d] Waiting for server to fulfill request... \n", counter*20);
            mexEvalString("drawnow update;");
        }

		Sleep(20);
		
		if(counter > 500)
		{
			return FALSE;
		}
	}
	
    client->RequestIsReady = 0;
    
	return TRUE;
}
