function computeMaxRepeat(len,l_q, Lists)
done = 0
% Initialize pointer to all lists
for i = 1:l_q
	p[i] = 0;
	end[i] = length(Lists(l_q))
end 
  
max_repeat = 0;
NN = 0;

while !done 
	done = 1
	min_el = infinity
	second_min = infinity
	R = 0
	active_lists = 0    
    % go through all lists and find the current minimum element
	for i = 1:l_q
		% Check if this list is exhausted
		if p[i] < end[i]
			done = 0
			active_lists++
			if Lists(i)(p[i]) == min_el
            % this list also has the same element as the current minimum. 
            % This represents an intersection
				minL[R] = i 
				R++
			elseif Lists(i)(p[i]) < min_el
				second_min = min_el 
				min_el = Lists(i)(p[i]) 
				R = 0
				minL[R] = i 
				R++
			elseif Lists(i)(p[i]) < second_min
				second_min = Lists(i)(p[i]) 
			end
		end
end
    % now update the current best
	if max_repeat < R
		Rmax = R
		NN = min_el
    end
    % now update the pointers
	for i = 1:R
		while (p[i] < end[i]) && Lists(i)(p[i]) < second_min
		  p[i]++
		end
    end
    % Some exit conditions	to terminate the search
	if (active_lists < max_repeat) || (max_repeat == l_q) || (max_repeat == len)
		break
	end
    end
    return NN, Rmax
end