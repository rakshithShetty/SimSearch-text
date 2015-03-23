clc; clear all;

clear; clc;close all
d = 100 * 2.^[0:2:14];
type = 'Rand_';

for i = 1: length(d)
    fid = fopen(['ApproxResults2/Fast_SearchResult' type num2str(d(i)) '.txt']);    
    for j = 1:11
        fgetl(fid);
    end
    q = fscanf(fid,'Query Count = %d\n',1);
    dExact = fscanf(fid,'NN for tweet %*d, is %*d d = %f\n',q);
    fclose(fid);

    fid = fopen(['ApproxResults2/Approx20_SearchResult' type num2str(d(i)) '.txt']);    
    for j = 1:11
        fgetl(fid);
    end
    q = fscanf(fid,'Query Count = %d\n',1);
    dApprox = fscanf(fid,'NN for tweet %*d, is %*d d = %f\n',1000);
    fclose(fid);
    clear diff;
    diff = (acos(min(dApprox,1))+eps)./(acos(min(dExact,1))+eps);
    if(~isempty(diff))
        maxd(i) = max(diff);
        md(i) = mean(diff);
    end
    
    fid = fopen(['ApproxResults2/Approx_SearchResult' type num2str(d(i)) '.txt']);
    for j = 1:11
        fgetl(fid);
    end
    q = fscanf(fid,'Query Count = %d\n',1);
    dApprox = fscanf(fid,'NN for tweet %*d, is %*d d = %f\n',1000);
    fclose(fid);
    clear diff;
    diff = (acos(min(dApprox,1))+eps)./(acos(min(dExact,1))+eps);
    if(~isempty(diff))
        maxd2(i) = max(diff);
        md2(i) = mean(diff);
    end
end
md(md==0) =1;
md2(md2==0) =1;
maxd(maxd==0) =1;
maxd2(maxd2==0) =1;
%figure;semilogx(d,(md-1)*100, d,(md2-1)*100);
% title('Mean Approximation Error vs dimension of the vocablary: InFreq terms');
% ylabel('Mean approximation error in  %');

figure;semilogx(d,(maxd-1)*100, d,(maxd2-1)*100);
title(['Max Approximation Error vs dimension of the vocablary:' type(1:end-1) ' terms']);
ylabel('Max approximation error in  %');

xlabel('Dimension in log scale');
legend('Approx - 20%', 'Approx - 50%'); 