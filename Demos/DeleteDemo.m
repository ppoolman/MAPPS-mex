tic;

client.name = 'matlab_2';

ProjectName = mxGetProjectName(client);
startTime = mxGetStartTime(client);
disp(['Project name is ', ProjectName]);
disp(['Start time (FILETIME) is ',num2str(startTime)]);

BusName = 'MyBus';
dT = 10;    % 10ms

mxDeleteBus(client, BusName);

Timestamps = 0:dT:60*1000;
Signal = cos(2 * pi() * Timestamps / 1000);

n = length(Timestamps);
mxCreateBus(client, n, BusName, {'Field1'}, {'double'});

Data = mxGetBuses(client, {''}, {BusName});

mxSetTime(client, Timestamps(end) / 1000);

mxUpdateBus(client, Timestamps, struct(BusName, struct('Field1', Signal)));

t = Timestamps(end) - dT / 2;
nI = 5;
for i = n:-nI:-nI
    Data = mxGetBuses(client, {''}, {BusName});
    startIndex = find(Data.(BusName).MAPPS_time >= t, 1);
    if ~isempty(startIndex)
        endIndex = length(Data.(BusName).MAPPS_time);
%         sprintf('Begin time = %d, End time = %d, # of timeslices = %d, startIndex = %d, endIndex = %d, count = %d', ...
%                 Data.(BusName).MAPPS_time(1), Data.(BusName).MAPPS_time(end), length(Data.(BusName).MAPPS_time), ...
%                 startIndex, endIndex, endIndex - startIndex + 1)
        mxDeleteByIndex(client, BusName, startIndex, endIndex);
    end
    t = t - dT * nI;
end

toc;