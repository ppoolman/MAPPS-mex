AddSignalBusName = 'TestBus';

[Time Buses] = mxListBuses;

while ~isfield(Buses, AddSignalBusName)
    pause(1);
    [Time Buses] = mxListBuses;
end

CurrentTimestamps = [];

while 1
    Data = mxGetBuses({''}, {AddSignalBusName});
    NewTimestamps = setdiff(Data.(AddSignalBusName).MAPPS_time, CurrentTimestamps);
    if ~isempty(NewTimestamps)
        disp(['Timestamp(s) added = ', num2str(NewTimestamps')]);
        CurrentTimestamps = union(NewTimestamps, CurrentTimestamps);
    else
        DeletedTimestamps = setdiff(CurrentTimestamps, Data.(AddSignalBusName).MAPPS_time);
        if ~isempty(DeletedTimestamps)
            disp(['Timestamp(s) deleted = ', num2str(DeletedTimestamps')]);
            CurrentTimestamps = setxor(DeletedTimestamps, CurrentTimestamps);
        end
    end
    
    disp(['Current timestamp(s) = ', num2str(reshape(CurrentTimestamps, 1, length(CurrentTimestamps)))]);
    disp(' ');
    
    pause(1);
end

