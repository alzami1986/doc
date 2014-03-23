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
nameAl      = 'multi';
legLocation = 'EastOutside';
nameTest    = 'total';%   {'vid'}
nTest       = 4;

compute    = 1;
saveResult = 0;
loadResult = 0;
graph      = 1;
graphLeg   = 0;

%-- Other parameters
time          = 1;
nFrames       = 15;
nReplications = 50;

%-- Graph width & ratio
typeDoc = elsevier; % {ieee, elsevier};
ratio   = 1.25;% 1.75; % 2.5; {1.75, 2.5}

%---------------------------- Graphics parameters -----------------------------%
%-- Graphs width
switch typeDoc
    case elsevier
        errWidth = 16.64/3;
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
addrCls   = [ 0, cumsum(sizeClsDb) ];

if compute
    %--------------------------------------------------------------------------%
    %----------------------------- Initialization -----------------------------%
    predi    = zeros(nPatterns, 1);
    truec    = zeros(nPatterns, 1);
    
    if strcmp(nameSc, 'add')
        graphCmc = zeros(2*nTest,       time+1);
        cRate    = zeros(nReplications, time+1);
    else
        graphCmc = zeros(2*nTest,       nClasses);
        cRate    = zeros(nReplications, nClasses);
    end
    
    textLegend = '';

    %-- Confidence interval
    confInterval = 0.95;  %-- 1 - alpha/2, or alpha = 10%
    pValue       = tinv(confInterval, nReplications-1) / sqrt(nReplications);

    %--------------------------------------------------------------------------%
    %-------------------- Calcul des points du graphiques ---------------------%
    %--------------------------------------------------------------------------%
    nSamples  = nClasses;
    addrGraph = 0;
    
	tTest = 1;
    while tTest <= nTest

        %-- Name parameters & legend in function of the current curve
        name = util_getName(nameTest, tTest);

        %-- Result file loaded
        nameFile = sprintf('../savedStuff/%s_%s%s%s%s_%dBlocks_wd%d.result',...
                 nameDb, nameSc, name.ln, name.hp, name.al, nBlocks, name.width)

        result   = util_readResults(nameFile, nBlocks, nReplications);

        %-- Graph points themselves
        t = time;

        for r=1:nReplications 

            if strcmp(nameSc, 'add')
                nClassOk = zeros(1, time+1);
            else
                nClassOk = zeros(1, nClasses);
            end
            
            nPatterns = result.nPatternsTest(t,r);
            predi = result.predisemble(1:nPatterns, t, r) +1;
            truec = result.trueClasses(1:nPatterns, t, r) +1;

%             predi = predi(predi>0);
%             truec = truec(truec>0);
            
            %-- Classes activity and sizes
            activeCls        = zeros(1,nClasses);
            activeCls(truec) = 1;
            nActive          = sum(activeCls);
            [tmp activeCls]  = sort(activeCls, 'descend');
            activeCls        = activeCls(1:nActive);
            
            addrCls          = [0 cumsum(sizeClsDb(activeCls))];

            
            %-------- Build histogram - each sample is a video sequence -------%
            for sample = 1:nActive
                
                %-- Build the histogram
                videoHist = zeros(1, nClasses);
                for f = 1:nFrames
                    addr                   = addrCls(sample) +f;
                    videoHist(predi(addr)) = videoHist(predi(addr)) +1;
                end

                %-- Sort the histogram
                [sortHist, rnk] = sort(videoHist, 'descend');
                
                %-- Find rank
                findRk          = eq(rnk, truec(addr));
                
                %-- Check for each rank
                for rk = 1:nActive
                    withinRange = sum( findRk(1:rk) );
                    if( withinRange )
                        nClassOk(rk) = nClassOk(rk)+1;
                    end
                end
            end
            
            %---------------- Finds the classification rate ---------------%
            nClassOk(nActive) = nActive;
            cRate(r,:)        = nClassOk/nActive;
        end  %-- for : nRep

        graphCmc(tTest*2-1, :) = mean(cRate  ) *100;
        graphCmc(tTest*2  , :) = std (cRate,0) *100 *pValue;
        
        tTest = tTest+1;
    end
end
%------------------------------------------------------------------------------%
%------------------------------ Saving & loading ------------------------------%
if saveResult
    nameFile = sprintf('../savedStuff/%s_%s%sCmc_t%d_f%d.mat', ...
						nameDb, nameSc, nameAl, time, nFrames);
    save(nameFile, 'graphCmc', '-mat');
end

if loadResult
    nameFile = sprintf('../savedStuff/%s_%s%sCmc_t%d_f%d.mat', ...
						nameDb, nameSc, nameAl, time, nFrames);
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

    util_graphCmcTime(graphCmc, nFrames, nTest);
    
%     hold on
%     for g = 1:nCurves
%         plot([1:nClasses], graphCmc(g*2-1, :));
%     end
%     hold off
    
    nameFile = sprintf('export_fig ../figures/%s_%s%sCmc_t%d_f%d -pdf', ...
                       nameDb, nameSc, nameAl, time, nFrames);
    eval(nameFile);
end
fprintf('/*----------------- End of results -----------------*/\n');