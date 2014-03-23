function [dbase] = util_readCfg(nameFile)

f = fopen(nameFile,'r');

    temp      = fread(f, 1, 'int32')+1;
    temp      = fread(f, 1, 'int32')+1;
    nClasses  = fread(f, 1, 'int32');
    nFeatures = fread(f, 1, 'int32');

    artmap = struct( ...
     'nClasses',    nClasses,                             ...
     'nFeatures',   nFeatures,                            ...
     'sizeNn',      zeros(1,                    sizeEns), ...
     'sizeClasses', zeros(nClasses,             sizeEns), ...
     'h',           zeros(4,                    sizeEns), ...
     'W',           zeros(maxSize, nFeatures*2, sizeEns), ...
     'normW',       zeros(maxSize, 1,           sizeEns), ...
     'normWalpha',  zeros(maxSize, 1,           sizeEns), ...
     'Wab',         zeros(maxSize, nClasses,    sizeEns)  ...
    );

    dbase.nPatterns = fread(f, 1, 'int32');%   nombre d'exemples    --%
    dbase.nFeatures = fread(f, 1, 'int32');%   nombre de dimensions --%
    dbase.nClasses  = fread(f, 1, 'int32');%   nombre de classes    --%
    
fclose(f);