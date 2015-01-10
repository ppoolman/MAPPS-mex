client.name = 'matlab';

ImageFolder = 'C:\Users\ppoolman\eyesDx\Matlab data\MAPPS demos\OverlayDemo image files\';
CanvasName = 'security2';

AvailableCanvases = mxListCanvases(client);
if ~isstruct(AvailableCanvases) || ~isfield(AvailableCanvases, CanvasName)
    error([CanvasName, ' does not exist.']);
end

LR = imread([ImageFolder, 'Count.bmp']);
for i = 1:10
    L(i) = {imread([ImageFolder, 'Count_', num2str(i - 1), '_L.bmp'])};
end
for i = 1:10
    R(i) = {imread([ImageFolder, 'Count_', num2str(i - 1), '_R.bmp'])};
end

for i = 1:10
    LL(i) = {mxUpdateOverlay(client, CanvasName, L{i}, true, 'hwc')};
end
for i = 1:10
    RR(i) = {mxUpdateOverlay(client, CanvasName, R{i}, true, 'hwc')};
end

tic;

mxUpdateOverlay(client, CanvasName, LR, true, 'hwc');

for j = 1:50
    for i = 1:10
%         mxUpdateOverlay(client, CanvasName, L{i}, true, 'hwc');
        mxUpdateOverlay(client, CanvasName, LL{i}, false, 'cwh');
    end
    for i = 1:10
%         mxUpdateOverlay(client, CanvasName, R{i}, true, 'hwc');
        mxUpdateOverlay(client, CanvasName, RR{i}, false, 'cwh');
    end
end

mxUpdateOverlay(client, CanvasName, LR, true, 'hwc');

toc;
