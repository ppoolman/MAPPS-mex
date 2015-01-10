tic;

BusName_Ind = 'MyBus_forIndex';
BusName_Time = 'MyBus_forTime';
dT = 10;    % 10ms
 
mxDeleteBus(BusName_Ind);
mxDeleteBus(BusName_Time);

Timestamps = 0:dT:60*1000;
Signal = cos(2 * pi() * Timestamps / 1000);

n = length(Timestamps);
mxCreateBus(n, BusName_Ind, {'Field1_Ind'}, {'double'});
mxCreateBus(n, BusName_Time, {'Field1_Time'}, {'double'});

mxUpdateBus(Timestamps, struct(BusName_Ind, struct('Field1_Ind', Signal)));
mxUpdateBus(Timestamps, struct(BusName_Time, struct('Field1_Time', Signal)));

t = Timestamps(end) - dT / 2;
nI = 10;
for i = n:-nI:-nI
    Data = mxGetBuses({''}, {BusName_Ind});
    startIndex = find(Data.(BusName_Ind).MAPPS_time >= t, 1);
    if ~isempty(startIndex)
        endIndex = length(Data.(BusName_Ind).MAPPS_time);
        sprintf('Begin time = %d, End time = %d, # of timeslices = %d, startIndex = %d, endIndex = %d, count = %d', ...
                Data.(BusName_Ind).MAPPS_time(1), Data.(BusName_Ind).MAPPS_time(end), length(Data.(BusName_Ind).MAPPS_time), ...
                startIndex, endIndex, endIndex - startIndex + 1)
        mxDeleteByIndex(BusName_Ind, startIndex - 1, endIndex - 1);
    end
    
    Data = mxGetBuses({''}, {BusName_Time});
    startIndex = find(Data.(BusName_Time).MAPPS_time >= t, 1);
    if ~isempty(startIndex)
        startTime = Data.(BusName_Time).MAPPS_time(startIndex);
        endIndex = length(Data.(BusName_Time).MAPPS_time);
        endTime = Data.(BusName_Time).MAPPS_time(endIndex);
        sprintf('Start time = %d, End time = %d, # of timeslices = %d, startIndex = %d, endIndex = %d, count = %d', ...
                startTime, endTime, length(Data.(BusName_Time).MAPPS_time), ...
                startIndex, endIndex, endIndex - startIndex + 1)
        mxDeleteByTime(BusName_Time, startTime, endTime);
    end
    
    t = t - dT * nI;
end

toc;