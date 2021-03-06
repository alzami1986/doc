%------------------------------------------------------------------------------%
%-- Results: batch -vs- inremental for different settings
%-- Works with util_getName
%------------------------------------------------------------------------------%
fprintf('\n/*----------------- Results - CMC ------------------*/\n');

%-- DEFINE\
elsevier = 1; ieee = 2;    lnsc = 3;
%-- END DEFINE

%------------------------------------------------------------------------------%
%--------------------------- User-defined parameters --------------------------%
%     __
%    |  |
%    |  |
%   _|  |_
%   \    /
%    \  /
%     \/
%-- File name
nameDb      = 'cnrc64';
nameSc      = 'add';%   {add, upd}
nBlocks     =  10;
widthMc     =  100;
nameAl      = 'multi';
legLocation = 'EastOutside';
nameTest    = 'total';%   {'vid'}
typeTest    = 1;

compute    = 1;
saveResult = 0;
loadResult = 0;
graph      = 1;
graphLeg   = 0;

%-- Other parameters
widthFrames   = 5;
maxFrames     = 35;
nReplications = 50;

%-- Graph width & ratio
typeDoc = elsevier; % {ieee, elsevier};
ratio   = 1.75; % 2.5; {1.75, 2.5}

%---------------------------- Graphics parameters -----------------------------%
%-- Graphs width
switch typeDoc
    case elsevier
        errWidth = 16.64/2;
    case ieee
        errWidth = 8.96;
    case lnsc
        errWidth = 12.2;
end

%------------------- Computing separator and removed class --------------------%
%-- Read Db
nameFile  = sprintf('../database/%s/test.db',nameDb);
dbase     = util_readDb(nameFile);
nClasses  = dbase.nClasses;
nPatterns = dbase.nPatterns;


%-- Nb of patterns per classes & addresses
sizeClsDb = zeros(1, nClasses);
for i = 1:nPatterns
    sizeClsDb( dbase.tags(i) +1 ) = sizeClsDb( dbase.tags(i) +1 ) + 1;
end
addrCls   = [0, cumsum(sizeClsDb) ];

if compute
    %--------------------------------------------------------------------------%
    %----------------------------- Initialization -----------------------------%
    nCurves = maxFrames / widthFrames;
    graphCmc = zeros(nCurves*2, nClasses);
    predi    = zeros(nPatterns, 1);
    truec    = zeros(nPatterns, 1);
    
    cRate    = zeros(nReplications, nClasses);

    textLegend = '';

    %-- Confidence interval
    confInterval = 0.95;  %-- 1 - alpha/2, or alpha = 10%
    pValue       = tinv(confInterval, nReplications-1) / sqrt(nReplications);

    %--------------------------------------------------------------------------%
    %-------------------- Calcul des points du graphiques ---------------------%
    %--------------------------------------------------------------------------%
    %-- Name parameters & legend in function of the current curve
    name = util_getName(nameTest, typeTest);

    %-- Result file loaded
    nameFile = sprintf('../savedStuff/%s_%s%s%s%s_%dBlocks_wd%d.result',...
                    nameDb, nameSc, name.ln, name.hp, nameAl, nBlocks, widthMc);

    result   = util_readResults(nameFile, nBlocks, nReplications);

    %------------ Graph points - for all blocs & replications -------------%
    nSamples  = nClasses;
    addrGraph = 0;
    
    for nFrames = widthFrames:widthFrames:maxFrames

        %-- Graph points themselves
        t = nBlocks;

        for r=1:nReplications 

            nClassOk = zeros(1, nClasses);
            
            predi(1:nPatterns) = result.predisemble(1:nPatterns, t, r) +1;
            truec(1:nPatterns) = result.trueClasses(1:nPatterns, t, r) +1;

            %-------- Build histogram - each sample is a video sequence -------%
            for sample = 1:nSamples
                
                %-- Build the histogram
                cClass = sample
                videoHist = zeros(1, nClasses);
                for f = 1:nFrames
                    addr                   = addrCls(cClass) +f;
                    videoHist(predi(addr)) = videoHist(predi(addr)) +1;
                end

                %-- Sort the histogram
                [sortHist, rnk] = sort(videoHist, 'descend');
                
                %-- Find rank
                findRk          = eq(rnk, truec(addr));
                
                %-- Check for each rank
                for rk = 1:nClasses
                    withinRange = sum( findRk(1:rk) );
                    if( withinRange )
                        nClassOk(rk) = nClassOk(rk)+1;
                    end
                end
            end

            %---------------- Finds the classification rate ---------------%
            cRate(r,:) = nClassOk/nClasses;
%             bp=1;
        end  %-- for : nRep

        %-- Add
%         cRate(43,:) = [];
%         cRate(38,:) = [];
%         cRate(35,:) = [];
%         cRate(31,:) = [];
%         cRate(20,:) = [];
%         cRate(16,:) = [];
%         cRate(11,:) = [];
%         cRate(8,:) = [];
%         cRate(6,:) = [];

        %-- CMC curves
        addrGraph = addrGraph +1;

        graphCmc(addrGraph*2-1, :) = mean(cRate  ) *100;
        graphCmc(addrGraph*2  , :) = std (cRate,0) *100 *pValue;
    end
end
%------------------------------------------------------------------------------%
%------------------------------ Saving & loading ------------------------------%
if saveResult
    nameFile = sprintf('../savedStuff/%s_%s%sCmc.mat', nameDb, nameSc, nameAl);
    save(nameFile, 'graphCmc', '-mat');
end

if loadResult
    nameFile = sprintf('../savedStuff/%s_%s%sCmc.mat', nameDb, nameSc, nameAl);
    load(nameFile, 'graphCmc', '-mat');
end

%------------------------------------------------------------------------------%
%----------------------------------- Graphs -----------------------------------%
close all;

if graph
    temp = get(0,'MonitorPosition');
    sizeScrn = [-temp(1,1)+1 temp(1,4)];
    res = get(0,'ScreenPixelsPerInch')/2.56;

    width  = res*errWidth+2;   height = res*errWidth/ratio+2;
    posScrn = [0 sizeScrn(2)-height-104 width height];

    fig_1 = figure(1);     clf(fig_1);
    posPaper = [0 0 errWidth errWidth/ratio];
    set(fig_1, 'Position', posScrn, 'PaperUnits', 'centimeters', ...
               'PaperSize', [posPaper(3) posPaper(4)],...
               'PaperPosition', posPaper, 'Color', [1,1,1]);
           
    a(1) = axes('FontName', 'Times New Roman', 'YGrid', 'on', ...
                'Position', [0.09 0.155 0.75 0.82]);

    set(0,'CurrentFigure',1);

    util_graphCmc(graphCmc, widthFrames, maxFrames);
    
%     hold on
%     for g = 1:nCurves
%         plot([1:nClasses], graphCmc(g*2-1, :));
%     end
%     hold off
    
    nameFile = sprintf('export_fig ../figures/%s_%s%sCmc -pdf', ...
                       nameDb, nameSc, nameAl);
    eval(nameFile);
end
fprintf('/*----------------- End of results -----------------*/\n');
