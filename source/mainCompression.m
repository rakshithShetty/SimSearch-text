clc;clear; close all
I_orig = imread('T-61_5100_city_orig_3.png');

imSz=size(I_orig);
imgdct = dct2(I_orig);

dctVec = imgdct(:);

[dctVecSort, srtidx] = sort(abs(dctVec));

err = (cumsum(dctVecSort.^2)./(dctVecSort'*dctVecSort))*100;

figure; plot(err,[length(err):-1:1]);

for i = 2:2:6

    last = find(err>i,1);
    truncVec = zeros(length(dctVec),1);
    truncVec(srtidx(last:end)) = dctVec(srtidx(last:end));
    
    reconstImg = uint8(idct2(reshape(truncVec,imSz(1),imSz(2))));
    
    figure;imshow(reconstImg);
    
    nC = length(dctVec) - (last -1);
    
    c = length(dctVec)/(nC);
    
    fprintf('For error = %d%% No of components kept = %d, and C = %f\n',i,nC,c); 
    imwrite(reconstImg,['Compressed_' num2str(i) 'perc.png']);
end
