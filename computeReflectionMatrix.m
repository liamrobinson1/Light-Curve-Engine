function G = computeReflectionMatrix(sun_vectors, viewer_vectors, normal_vectors)
    G = zeros(length(sun_vectors), length(normal_vectors));
    for i = 1:length(sun_vectors)
        for j = 1:length(normal_vectors)
            S_dot_N = dot(sun_vectors(i, :), normal_vectors(j, :));
            V_dot_N = dot(viewer_vectors(i, :), normal_vectors(j, :));
            if S_dot_N > 0 && V_dot_N > 0
                G(i, j) = S_dot_N * V_dot_N;
        end
    end
end
