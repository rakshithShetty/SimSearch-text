%% function analyzeTweets
clc; clear; close all

%A = readtable('vocabMasterDb.txt','ReadVariableNames',0);


% fid = fopen('vocabMasterDb.txt');
% 
% a = fscanf(fid,'%d',1);
% 
% Freqs = zeros(a,1);
% Freqs = fscanf(fid,'%*s %d',a);

load ReadFile.mat
%plot((((1:length(Freqs)))), log(log((Freqs))));
loglog(Freqs);

title('No of tweets term i occurs in loglog scale ');
xlabel('term index');
ylabel(' No of tweets the term occurs in');


[a,b]=hist(Freqs,unique(Freqs));

%figure; plot(log(log(b)),(log(a)));
figure;loglog(b,a)
title('Distribution of n_j in loglog scale ');
xlabel('No of tweets n');
ylabel('No of terms');


figure;loglog(b,cumsum(a));

title('Cumulative distribution of n_j in loglog scale ');
xlabel('No of tweets n');
ylabel('No of terms');


%fclose(fid);