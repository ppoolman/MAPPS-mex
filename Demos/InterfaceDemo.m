BusName_1 = 'MyBus_1';
dT_1 = 10;    % 10ms
c_1 = 0.2;
BusName_2 = 'MyBus_2';
dT_2 = 50;    % 50ms
c_2 = 0.1;
 
mxDeleteBus(BusName_1);
mxDeleteBus(BusName_2);

[Time Avail] = mxListBuses;
% n_1 = (Time.Global_MAPPS_Time.EndTime - Time.Global_MAPPS_Time.StartTime) / dT_1 + 1;
n_1 = (double(Time.Global_MAPPS_Time.EndTime) - double(Time.Global_MAPPS_Time.StartTime)) / dT_1 + 1;
t_old_1 = Time.Global_MAPPS_Time.StartTime;
% n_2 = (Time.Global_MAPPS_Time.EndTime - Time.Global_MAPPS_Time.StartTime) / dT_2 + 1;
n_2 = (double(Time.Global_MAPPS_Time.EndTime) - double(Time.Global_MAPPS_Time.StartTime)) / dT_2 + 1;
t_old_2 = Time.Global_MAPPS_Time.StartTime;

mxCreateBus(n_1, BusName_1, {'Field1'}, {'double'});
mxCreateBus(n_2, BusName_2, {'Field1' 'Field2'}, {'float' 'int'});

level = 0;
while 1
    t = mxGetTime * 1000;
%     if t > t_old_1 + dT_1
    if double(t) > double(t_old_1) + dT_1    
%         timestamps_1 = t_old_1:dT_1:t;
        timestamps_1 = double(t_old_1):dT_1:double(t);
        Bus_1_Field_1 = cos(c_1 * 2 * pi() * timestamps_1 / 1000);
        t_old_1 = timestamps_1(end);
        
        mxUpdateBus(timestamps_1, struct(BusName_1, struct('Field1', Bus_1_Field_1)));
    end
    
%     if t > t_old_2 + dT_2  
    if double(t) > double(t_old_2) + dT_2
%         timestamps_2 = t_old_2:dT_2:t;
        timestamps_2 = double(t_old_2):dT_2:double(t);
        Bus_2_Field_1 = cos(c_2 * 2 * pi() * timestamps_2 / 1000);
        Bus_2_Field_2 = repmat(level, length(timestamps_2), 1);
        t_old_2 = timestamps_2(end);
        
        mxUpdateBus(timestamps_2, struct(BusName_2, struct('Field1', single(Bus_2_Field_1), 'Field2', Bus_2_Field_2)));
    end
    
    if t < t_old_1
        Data = mxGetBuses({''}, {BusName_1});
        startIndex = find(Data.(BusName_1).MAPPS_time >= t, 1);
        if ~isempty(startIndex)
            endIndex = length(Data.(BusName_1).MAPPS_time);
            sprintf('   Begin time = %d, End time = %d, # of timeslices = %d, startIndex = %d, endIndex = %d, count = %d', ...
                    Data.(BusName_1).MAPPS_time(1), Data.(BusName_1).MAPPS_time(end), length(Data.(BusName_1).MAPPS_time), ...
                    startIndex, endIndex, endIndex - startIndex + 1)
            if Data.(BusName_1).MAPPS_time(1) > Data.(BusName_1).MAPPS_time(end)
                stop = 1;
            end
            mxDeleteByIndex(BusName_1, startIndex - 1, endIndex - 1);
        end
        c_1 = c_1 * 1.2;
        t_old_1 = t;
    end
    
    if t < t_old_2
        Data = mxGetBuses({''}, {BusName_2});
        startIndex = find(Data.(BusName_2).MAPPS_time >= t, 1);
        if ~isempty(startIndex)
            endIndex = length(Data.(BusName_2).MAPPS_time);
            mxDeleteByIndex(BusName_2, startIndex - 1, endIndex - 1);
        end
        c_2 = c_2 * 1.2;
        level = level + 1;
        t_old_2 = t;
    end
    
    pause(0.1);
    
end