ET_busname = 'eye_tracking';
IntersectionX_fieldname = 'ObjectIntersectionX';
IntersectionY_fieldname = 'ObjectIntersectionY';
IntersectedCanvas_fieldname = 'ObjectIntersectionName';
% IntersectedCanvas_fieldname = 'IntersectionName';
% IntersectedCanvas_fieldname = [];

ETCanvas_names = [];  % if canvas names are different from names of intersected screens used in ET data, otherwise set to []
% ETCanvas_names = {'Screen1' 'Screen2' 'Screen3'};  % if canvas names are different from names of intersected screens used in ET data, otherwise set to []
% ETCanvas_names = {'left di' 'Center' 'Right d'};  % if canvas names are different from names of intersected screens used in ET data, otherwise set to []
% ETCanvas_names = {'Camera'};  % if canvas names are different from names of intersected screens used in ET data, otherwise set to []

duration = 5;   % in secs.

drawCrossHair = true;
r = 50;         % in pixels
nP = 40;        % # of points for crosshair circle

%%

for i = 1:nP
    DX(i, 1) = r * cos(i / (nP - 1) * 2 * pi);
    DY(i, 1) = r * sin(i / (nP - 1) * 2 * pi);
end

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
for j = 1:nC
    try 
        close(H{j});
    catch exception
        if strcmp(exception.identifier, 'MATLAB:close:InvalidFigureHandle') || ...
           strcmp(exception.identifier, 'MATLAB:undefinedVarOrClass') || ...
           strcmp(exception.identifier, 'MATLAB:badsubscript')
        else
            throw(exception);
        end
    end
        
    H{j} = figure('Color', 'k');
end

if ~isempty(ETCanvas_names)
    [TF CanvasIndex] = ismember(Data.(ET_busname).(IntersectedCanvas_fieldname), ETCanvas_names);
else
    if ~isempty(IntersectedCanvas_fieldname)
        [TF CanvasIndex] = ismember(Data.(ET_busname).(IntersectedCanvas_fieldname), CanvasNames);
    else
        CanvasIndex = ones(nT, 1);
    end
end

%%

t_old = -1;
for i = 1:nC
    clearCanvas(i) = false;
end
X = zeros(1000, 1);
indX = 1;
Y = zeros(1000, 1);
indY = 1;
while 1
    t = mxGetTime;
    
    if t ~= t_old
        nextCanvas = false;
        onCanvas = false;
        for i = 1:nC
            usedCanvas(i) = false;
        end
        
        ind_begin = find(Data.(ET_busname).MAPPS_time > (t - duration) * 1000, 1);
        ind_end = find(Data.(ET_busname).MAPPS_time > t * 1000, 1);
        
        for i = ind_begin:ind_end          
            if (onCanvas || i ~= ind_end && (CanvasIndex(i) == CanvasIndex(i + 1) && CanvasIndex(i) > 0)) && ...
               Data.(ET_busname).(IntersectionX_fieldname)(i) ~= 0 && Data.(ET_busname).(IntersectionY_fieldname)(i) ~= 0
                X(indX:indX + 1, 1) = Data.(ET_busname).(IntersectionX_fieldname)(i);
                Y(indY:indY + 1, 1) = Data.(ET_busname).(IntersectionY_fieldname)(i);
                indX = indX + 1;
                indY = indY + 1;
                
                onCanvas = true;
            end
            
            if i == ind_end || CanvasIndex(i) ~= CanvasIndex(i + 1) || ...
               Data.(ET_busname).(IntersectionX_fieldname)(i) == 0 || Data.(ET_busname).(IntersectionY_fieldname)(i) == 0     
                nextCanvas = true;
            end
            
            if nextCanvas && onCanvas
                figure(H{CanvasIndex(i)});
                if usedCanvas(CanvasIndex(i))
                    hold on;
                else
                    hold off;
                end
                plot(X(1:indX), Y(1:indY), 'Color', 'r', 'LineWidth', 2);
                hold off;
                nextCanvas = false;
                onCanvas = false;
                usedCanvas(CanvasIndex(i)) = true;
                indX = 1;
                indY = 1;
            end
            
        end
        
        if ~isempty(ind_end) && drawCrossHair && CanvasIndex(ind_end) > 0 && ...
           Data.(ET_busname).(IntersectionX_fieldname)(ind_end) ~= 0 && Data.(ET_busname).(IntersectionY_fieldname)(ind_end) ~= 0
            figure(H{CanvasIndex(ind_end)});
            if usedCanvas(CanvasIndex(ind_end))
                hold on;
            else
                hold off;
            end
            plot(DX + Data.(ET_busname).(IntersectionX_fieldname)(ind_end), ...
                 DY + Data.(ET_busname).(IntersectionY_fieldname)(ind_end), ...
                 'Color', 'y', 'LineWidth', 3);
            hold off;
            usedCanvas(CanvasIndex(ind_end)) = true;
        end
        
        for j = 1:nC
            if usedCanvas(j)
                figure(H{j});
                
                X = [1; Canvases.(CanvasNames{j}).Width; Canvases.(CanvasNames{j}).Width; 1; 1];
                Y = [Canvases.(CanvasNames{j}).Height; Canvases.(CanvasNames{j}).Height; 1; 1; Canvases.(CanvasNames{j}).Height]; 
                hold on;
                plot(X(1:5), Y(1:5), 'Color', 'y', 'LineWidth', 1);
                hold off;
                
                axis off;
                axis equal;
                axis([1 Canvases.(CanvasNames{j}).Width 1 Canvases.(CanvasNames{j}).Height]);
         
                im = getframe(gca);
                mxUpdateOverlay(CanvasNames{j}, im.cdata, false, 'hwc');

                usedCanvas(j) = false;
                clearCanvas(j) = true;
            elseif clearCanvas(j)
                Blank = zeros(3, Canvases.(CanvasNames{j}).Width, Canvases.(CanvasNames{j}).Height);
                Blank(1, 1:Canvases.(CanvasNames{j}).Width, 1:2) = 255;
                Blank(1, 1:Canvases.(CanvasNames{j}).Width, Canvases.(CanvasNames{j}).Height - 1:Canvases.(CanvasNames{j}).Height) = 255;
                Blank(1, 1:2, 1:Canvases.(CanvasNames{j}).Height) = 255;
                Blank(1, Canvases.(CanvasNames{j}).Width - 1:Canvases.(CanvasNames{j}).Width, 1:Canvases.(CanvasNames{j}).Height) = 255;
                mxUpdateOverlay(CanvasNames{j}, Blank, false, 'cwh');
            end
        end
        
        t_old = t;
    
    end
    
    pause(0.01);
     
end
