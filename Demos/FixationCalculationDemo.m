maximumRadius = 0.02;
minimumDuration = 0.3;
maximumOutlierTime = 0.06;

%%

ET_busname = 'eye_tracking';
Time_fieldname = 'MAPPS_time';
timeMultiplier = 0.001;
IntersectionX_fieldname = 'ObjectIntersectionX';
IntersectionY_fieldname = 'ObjectIntersectionY';
IntersectedCanvas_fieldname = 'ObjectIntersectionName';
% IntersectedCanvas_fieldname = 'IntersectionName';

% ETCanvas_names = {'Screen1' 'Screen2' 'Screen3'};  % if canvas names are different from names of intersected screens used in ET data, otherwise set to []
ETCanvas_names = {'Camera'};  % if canvas names are different from names of intersected screens used in ET data, otherwise set to []

if ~isempty(IntersectedCanvas_fieldname)
    Data = mxGetBuses({IntersectionX_fieldname, ...
                       IntersectionY_fieldname, ...
                       IntersectedCanvas_fieldname}, ...
                      repmat({ET_busname}, 3, 1));
else
    Data = mxGetBuses({IntersectionX_fieldname, ...
                       IntersectionY_fieldname}, ...
                      repmat({ET_busname}, 2, 1));
end

Canvases = mxListCanvases;

%%

nT = length(Data.(ET_busname).(IntersectionX_fieldname));

CanvasNames = fieldnames(Canvases);
nC = length(CanvasNames);
for i = 1:nC
    CanvasWidth(i) = Canvases.(CanvasNames{i}).Width;
    CanvasHeight(i) = Canvases.(CanvasNames{i}).Height;
end

if ~isempty(ETCanvas_names)
    [TF CanvasIndex] = ismember(Data.(ET_busname).(IntersectedCanvas_fieldname), ETCanvas_names);
else
    [TF CanvasIndex] = ismember(Data.(ET_busname).(IntersectedCanvas_fieldname), CanvasNames);
end

%%

tic;

nData = 0;

nGazeSinceLastFix = 0;
GazeSinceLastFix_CanvasIndex = zeros(nT, 1);
GazeSinceLastFix_X = zeros(nT, 1);
GazeSinceLastFix_Y = zeros(nT, 1);
GazeSinceLastFix_Time = zeros(nT, 1);
GazeSinceLastFix_startTestIndex = zeros(1, nC);
GazeSinceLastFix_lastIndex = ones(1, nC);
GazeSinceLastFix_testIndexOffset = zeros(1, nC);
GazeSinceLastFix_lastClusterIndex = zeros(1, nC);

CandidateFix_initialStartIndex = zeros(1, nC);
CandidateFix_initialEndIndex = zeros(1, nC);
CandidateFix_startIndex = zeros(1, nC);
CandidateFix_endIndex = zeros(1, nC);
CandidateFix_ClusterMask(1, nC) = {[]};

inFix = false;
endFix = false;
nFix = 0;
offset = 0;
Fix_CanvasIndex = zeros(nT, 1);
Fix_StartIndex = zeros(nT, 1);
Fix_EndIndex = zeros(nT, 1);
Fix_MidPointX = zeros(nT, 1);
Fix_MidPointX_Normalized = zeros(nT, 1);
Fix_MidPointY = zeros(nT, 1);
Fix_MidPointY_Normalized = zeros(nT, 1);
Fix_StartTime = zeros(nT, 1);
Fix_EndTime = zeros(nT, 1);
Fix_ClusterMask(nT, 1) = {[]};

nI_max = 0;

for i = 1:nT
    
    nData = nData + 1;
    
    canvasIndex = CanvasIndex(i);
    nGazeSinceLastFix = nGazeSinceLastFix + 1;
    GazeSinceLastFix_Time(nGazeSinceLastFix) = double(Data.(ET_busname).(Time_fieldname)(i)) * timeMultiplier;
    x = Data.(ET_busname).(IntersectionX_fieldname)(i);
    y = Data.(ET_busname).(IntersectionY_fieldname)(i);
    if canvasIndex > 0 && x > 0 && y > 0
        GazeSinceLastFix_CanvasIndex(nGazeSinceLastFix) = canvasIndex;
        GazeSinceLastFix_X(nGazeSinceLastFix) = x / CanvasHeight(canvasIndex);
        GazeSinceLastFix_Y(nGazeSinceLastFix) = y / CanvasHeight(canvasIndex);
        if GazeSinceLastFix_startTestIndex(canvasIndex) == 0 || ...
           GazeSinceLastFix_Time(nGazeSinceLastFix) - GazeSinceLastFix_Time(GazeSinceLastFix_lastIndex(canvasIndex)) > maximumOutlierTime
            GazeSinceLastFix_startTestIndex(canvasIndex) = nGazeSinceLastFix;
            GazeSinceLastFix_testIndexOffset(canvasIndex) = 0;
        end
        GazeSinceLastFix_lastIndex(canvasIndex) = nGazeSinceLastFix;
    else
        GazeSinceLastFix_CanvasIndex(nGazeSinceLastFix) = 0;
    end
       
    for j = 1:nC
        if CandidateFix_initialStartIndex == 0  % keep testing for the onset of fixations on canvases other than the canvas with current fixation
            startIndex_temp = GazeSinceLastFix_startTestIndex(j);
            startIndex_orig = startIndex_temp;
            if startIndex_temp > 0 
                CanvasMask = find(GazeSinceLastFix_CanvasIndex(startIndex_temp:nGazeSinceLastFix) == j) + startIndex_temp - 1;
                CanvasMask_orig = CanvasMask;
                nI = length(CanvasMask);
                nI_orig = nI;
                endIndex_temp = CanvasMask(end);
                testIndexOffset_orig = GazeSinceLastFix_testIndexOffset(j);
                doneTest = false;
                while GazeSinceLastFix_Time(endIndex_temp) - GazeSinceLastFix_Time(startIndex_temp) >= minimumDuration
                    doneTest = true;
                    
                    if testIndexOffset_orig == 0
                        testIndexOffset = find(GazeSinceLastFix_Time(CanvasMask(2:end)) - GazeSinceLastFix_Time(startIndex_temp) >= minimumDuration, 1, 'first');
                    else
                        testIndexOffset = testIndexOffset_orig + nI - nI_orig;
                    end
                    if isempty(testIndexOffset)
                        n = 1;
                        sum_MidPointX = 0;
                        sum_MidPointY = 0;
                        testIndexOffset = nI + 1;
                    elseif testIndexOffset == nI    % to handle the case of nI not increasing after new data point is added and testIndexOffset_orig ~= 0
                        break;
                    else
                        n = testIndexOffset;
                        if n == 1
                            sum_MidPointX = 0;
                            sum_MidPointY = 0;
                        else
                            sum_MidPointX = sum(GazeSinceLastFix_X(CanvasMask(2:n)));
                            sum_MidPointY = sum(GazeSinceLastFix_Y(CanvasMask(2:n)));
                        end
                        testIndexOffset = testIndexOffset + 1;
                    end
                    
% REMOVE LATER - begin
                    if nI > nI_max
                        nI_max = nI;
                    end
% REMOVE LATER - end

                    allOutside = false;
                    for k = testIndexOffset:nI
                        n = n + 1;
                        sum_MidPointX = sum_MidPointX + GazeSinceLastFix_X(CanvasMask(k));
                        sum_MidPointY = sum_MidPointY + GazeSinceLastFix_Y(CanvasMask(k));
                        midPointX = (GazeSinceLastFix_X(startIndex_temp) + sum_MidPointX) / n;
                        midPointY = (GazeSinceLastFix_Y(startIndex_temp) + sum_MidPointY) / n;
                        DistanceSqr = (GazeSinceLastFix_X(CanvasMask(1:k)) - midPointX) .^ 2 + ...
                                      (GazeSinceLastFix_Y(CanvasMask(1:k)) - midPointY) .^ 2;
                        avgDistanceSqr = sum(DistanceSqr) / length(DistanceSqr);
                        TryMask = DistanceSqr < 2 * avgDistanceSqr;
                        TryMask(1) = true;
                        
                        n = sum(TryMask);
                        CanvasMask_try = CanvasMask(TryMask);
                        midPointX = sum(GazeSinceLastFix_X(CanvasMask_try)) / n;
                        midPointY = sum(GazeSinceLastFix_Y(CanvasMask_try)) / n;
                        DistanceSqr = (GazeSinceLastFix_X(CanvasMask(1:k)) - midPointX) .^ 2 + ...
                                      (GazeSinceLastFix_Y(CanvasMask(1:k)) - midPointY) .^ 2;
%                         TryMask = true(k, 1);

                        TestMask = DistanceSqr <= maximumRadius ^ 2;
                        while any(xor(TryMask, TestMask))
                            TryMask = TestMask;
                            n = sum(TryMask);
                            if n == 0
                                break;
                            end
                            CanvasMask_try = CanvasMask(TryMask);
                            midPointX = sum(GazeSinceLastFix_X(CanvasMask_try)) / n;
                            midPointY = sum(GazeSinceLastFix_Y(CanvasMask_try)) / n;
                            DistanceSqr = (GazeSinceLastFix_X(CanvasMask(1:k)) - midPointX) .^ 2 + ...
                                          (GazeSinceLastFix_Y(CanvasMask(1:k)) - midPointY) .^ 2;                            
                            TestMask = DistanceSqr <= maximumRadius ^ 2;
                        end
                        ClusterMask = CanvasMask(TestMask);
                        if length(ClusterMask) > 1 && ...
                           GazeSinceLastFix_Time(ClusterMask(end)) - GazeSinceLastFix_Time(ClusterMask(1)) >= minimumDuration && ...    
                           all(GazeSinceLastFix_Time(ClusterMask(2:end)) - GazeSinceLastFix_Time(ClusterMask(1:end - 1)) <= maximumOutlierTime)
                            CandidateFix_initialStartIndex(j) = ClusterMask(1);
                            CandidateFix_initialEndIndex(j) = ClusterMask(end);
                            CandidateFix_ClusterMask(j) = {ClusterMask};
                            inFix = true;
                            break;
                        end
                    end

                    if ~inFix && nI > 2
                        if startIndex_temp == startIndex_orig
                            GazeSinceLastFix_testIndexOffset(j) = nI;
                        end
                        CanvasMask(1) = [];
                        nI = length(CanvasMask);
                        startIndex_temp = CanvasMask(1);
                    else
                        break;
                    end
                end
                if ~inFix && doneTest
                    if ~isempty(ClusterMask)
                        firstClusterIndex = ClusterMask(1);
                        if GazeSinceLastFix_Time(firstClusterIndex) - GazeSinceLastFix_Time(GazeSinceLastFix_startTestIndex(j)) > 1.5 * minimumDuration
                            if GazeSinceLastFix_testIndexOffset(j) > 0
                                GazeSinceLastFix_testIndexOffset(j) = GazeSinceLastFix_testIndexOffset(j) - find(CanvasMask_orig == firstClusterIndex, 1, 'first');
                            end
                            GazeSinceLastFix_startTestIndex(j) = firstClusterIndex;
                            GazeSinceLastFix_lastClusterIndex(j) = 0;
                        else
                            lastClusterIndex = ClusterMask(end);
                            if GazeSinceLastFix_lastClusterIndex(j) == 0 || ...
                               GazeSinceLastFix_lastClusterIndex(j) < lastClusterIndex
                                GazeSinceLastFix_lastClusterIndex(j) = lastClusterIndex;
                            elseif GazeSinceLastFix_lastClusterIndex(j) == lastClusterIndex && ...
                                   GazeSinceLastFix_Time(nGazeSinceLastFix) - GazeSinceLastFix_Time(GazeSinceLastFix_lastClusterIndex(j)) > 1.5 * minimumDuration   
                                if GazeSinceLastFix_testIndexOffset(j) > 0
                                    GazeSinceLastFix_testIndexOffset(j) = GazeSinceLastFix_testIndexOffset(j) - find(CanvasMask_orig == lastClusterIndex, 1, 'first');
                                end
                                GazeSinceLastFix_startTestIndex(j) = lastClusterIndex;
                                GazeSinceLastFix_lastClusterIndex(j) = 0;
                            end
                        end
                    end        
                end
            end
        end
    end
    
    if inFix
        overlappingFixes = false;
        if sum(CandidateFix_startIndex > 0) > 1
            overlappingFixes = true;
        end
        
        if overlappingFixes
            disp('Add code to deal with overlapping candidate fixations');
            return;
        else
            canvasIndex = find(CandidateFix_initialStartIndex > 0, 1, 'first');
            if CandidateFix_startIndex(canvasIndex) == 0
                CandidateFix_startIndex(canvasIndex) = CandidateFix_initialStartIndex(canvasIndex);
                CandidateFix_endIndex(canvasIndex) = CandidateFix_initialEndIndex(canvasIndex);
            end
            startIndex = CandidateFix_startIndex(canvasIndex);
            CanvasMask = find(GazeSinceLastFix_CanvasIndex(startIndex:nGazeSinceLastFix) == canvasIndex) + startIndex - 1;
            nI = length(CanvasMask);
            midPointX = sum(GazeSinceLastFix_X(CanvasMask)) / nI;
            midPointY = sum(GazeSinceLastFix_Y(CanvasMask)) / nI;
            DistanceSqr = (GazeSinceLastFix_X(CanvasMask) - midPointX) .^ 2 + ...
                          (GazeSinceLastFix_Y(CanvasMask) - midPointY) .^ 2;
            TryMask = true(nI, 1);
            TestMask = DistanceSqr <= maximumRadius ^ 2;
            while any(xor(TryMask, TestMask))
                TryMask = TestMask;
                n = sum(TryMask);
                if n == 0
                    break;
                end
                CanvasMask_try = CanvasMask(TryMask);
                midPointX = sum(GazeSinceLastFix_X(CanvasMask_try)) / n;
                midPointY = sum(GazeSinceLastFix_Y(CanvasMask_try)) / n;
                DistanceSqr = (GazeSinceLastFix_X(CanvasMask) - midPointX) .^ 2 + ...
                              (GazeSinceLastFix_Y(CanvasMask) - midPointY) .^ 2;                            
                TestMask = DistanceSqr <= maximumRadius ^ 2;
            end
            ClusterMask = CanvasMask(TestMask);
            if length(ClusterMask) > 1 && ...
               ClusterMask(1) <= CandidateFix_endIndex(canvasIndex) && ...
               GazeSinceLastFix_Time(ClusterMask(end)) - GazeSinceLastFix_Time(ClusterMask(1)) >= 0.5 * minimumDuration && ...
               GazeSinceLastFix_Time(CanvasMask(end)) - GazeSinceLastFix_Time(ClusterMask(end)) < 1.5 * minimumDuration && ... 
               all(GazeSinceLastFix_Time(ClusterMask(2:end)) - GazeSinceLastFix_Time(ClusterMask(1:end - 1)) <= 1.5 * maximumOutlierTime)
                if GazeSinceLastFix_Time(ClusterMask(end)) - GazeSinceLastFix_Time(ClusterMask(1)) >= minimumDuration && ...
                   GazeSinceLastFix_Time(CanvasMask(end)) - GazeSinceLastFix_Time(ClusterMask(end)) < minimumDuration && ...
                   all(GazeSinceLastFix_Time(ClusterMask(2:end)) - GazeSinceLastFix_Time(ClusterMask(1:end - 1)) <= maximumOutlierTime)     
                    CandidateFix_startIndex(j) = ClusterMask(1);
                    CandidateFix_endIndex(j) = ClusterMask(end);
                    CandidateFix_ClusterMask(j) = {ClusterMask};
                end
            else
                fixCanvasIndex = canvasIndex;
                fixStartIndex = CandidateFix_startIndex(canvasIndex);
                fixEndIndex = CandidateFix_endIndex(canvasIndex);
                fixMidPointX = sum(GazeSinceLastFix_X(CandidateFix_ClusterMask{j})) / length(CandidateFix_ClusterMask{j});
                fixMidPointY = sum(GazeSinceLastFix_Y(CandidateFix_ClusterMask{j})) / length(CandidateFix_ClusterMask{j});
                fixStartTime = GazeSinceLastFix_Time(fixStartIndex);
                fixEndTime = GazeSinceLastFix_Time(fixEndIndex);
                fixClusterMask = CandidateFix_ClusterMask{j};
                CandidateFix_initialStartIndex(canvasIndex) = 0;
                CandidateFix_initialEndIndex(canvasIndex) = 0;
                CandidateFix_startIndex(canvasIndex) = 0;
                CandidateFix_endIndex(canvasIndex) = 0;
                endFix = true;
            end
        end
    end
    
    if endFix
        nFix = nFix + 1;
        Fix_CanvasIndex(nFix) = fixCanvasIndex;
        Fix_StartIndex(nFix) = fixStartIndex + offset;
        Fix_EndIndex(nFix) = fixEndIndex + offset;
        Fix_MidPointX(nFix) = fixMidPointX * CanvasHeight(fixCanvasIndex);
        Fix_MidPointX_Normalized(nFix) = Fix_MidPointX(nFix) / CanvasWidth(fixCanvasIndex);
        Fix_MidPointY(nFix) = fixMidPointY * CanvasHeight(fixCanvasIndex);
        Fix_MidPointY_Normalized(nFix) = fixMidPointY;
        Fix_StartTime(nFix) = fixStartTime;
        Fix_EndTime(nFix) = fixEndTime;
        Fix_ClusterMask(nFix) = {fixClusterMask + offset};
        offset = Fix_EndIndex(nFix); 
        if fixEndIndex < nGazeSinceLastFix
            nGazeSinceLastFix_temp = nGazeSinceLastFix - fixEndIndex;
            GazeSinceLastFix_CanvasIndex(1:nGazeSinceLastFix_temp) = GazeSinceLastFix_CanvasIndex(fixEndIndex + 1:nGazeSinceLastFix);
            GazeSinceLastFix_X(1:nGazeSinceLastFix_temp) = GazeSinceLastFix_X(fixEndIndex + 1:nGazeSinceLastFix);
            GazeSinceLastFix_Y(1:nGazeSinceLastFix_temp) = GazeSinceLastFix_Y(fixEndIndex + 1:nGazeSinceLastFix);
            GazeSinceLastFix_Time(1:nGazeSinceLastFix_temp) = GazeSinceLastFix_Time(fixEndIndex + 1:nGazeSinceLastFix);
            for j = 1:nC
                startIndex_temp = find(GazeSinceLastFix_CanvasIndex(fixEndIndex + 1:nGazeSinceLastFix) == j, 1, 'first');
                if isempty(startIndex_temp)
                    GazeSinceLastFix_startTestIndex(j) = 0;
                    GazeSinceLastFix_lastIndex(j) = 1;
                else
                    GazeSinceLastFix_startTestIndex(j) = startIndex_temp;
                    GazeSinceLastFix_lastIndex(j) = find(GazeSinceLastFix_CanvasIndex(fixEndIndex + 1:nGazeSinceLastFix) == j, 1, 'last');
                end
            end
            nGazeSinceLastFix = nGazeSinceLastFix_temp;
        else
            GazeSinceLastFix_startTestIndex = zeros(1, nC);
            GazeSinceLastFix_lastIndex = ones(1, nC);
            nGazeSinceLastFix = 0;
        end
        GazeSinceLastFix_testIndexOffset = zeros(1, nC);
        GazeSinceLastFix_lastClusterIndex = zeros(1, nC);
        inFix = false;
        endFix = false;
    end
        
end

toc;

%%

Fix = struct('DisplayIndex', [], ...
             'from_mapps_time', Fix_StartTime(1:nFix) * 1000, ...
             'To_MAPPS_time', Fix_EndTime(1:nFix) * 1000, ...
             'MidPointX', Fix_MidPointX_Normalized(1:nFix), ...
             'MidPointy', 1 - Fix_MidPointY_Normalized(1:nFix));
mxUpdateFixations(1, Fix);