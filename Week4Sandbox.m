CleanSlate

fname = 'test.mp4';
delete test.mp4
writerObj = VideoWriter(fname, 'MPEG-4');
writerObj.FrameRate = 10;
open(writerObj);

data_points = 500;
rng default

reference_model_file = "template_ico_sphere.obj";
command_file = "light_curve.lcc";
results_file = "light_curve.lcr";
dimensions = 15*60; %dimensions should be a multiple of 60
frame_rate = 500;
instances = 1;

%%%%% REFERENCE OBJECT HANDLING
reference_obj = readObj("models/" + reference_model_file, "false");
reference_obj = solveFaceAreas(reference_obj);
reference_obj = solveForUniqueNormalsAndAreas(reference_obj);
reference_obj = solveEGI(reference_obj);
ref_areas = vecnorm(reference_obj.EGI, 2, 2);
ref_normals = reference_obj.EGI ./ ref_areas;

t = linspace(0, 2 * pi, data_points)';
% sun_vectors = [cos(2*t) + 0*t, -cos(4*t) + 0*t, 0*t + sin(2*t)];
% viewer_vectors = [sin(t) + 0*t, cos(t) + 0*t, sin(5*t) + 0 * t];

sun_vectors = randUnitVectors(data_points);
viewer_vectors = randUnitVectors(data_points);

viewer_vectors = viewer_vectors ./ vecnorm(viewer_vectors, 2, 2) * 2;
sun_vectors = sun_vectors ./ vecnorm(sun_vectors, 2, 2) * 2;

% %%%%% GENERATING REFERENCE LIGHT CURVE
ref_light_curve = runLightCurveEngine(command_file, results_file, reference_model_file, instances, dimensions, data_points, ...
    sun_vectors, viewer_vectors, frame_rate);

normal_vectors = [getNormalVectors(1000, "spiral")]; %injecting real normals
G = computeReflectionMatrix(sun_vectors, viewer_vectors, normal_vectors);
% imagesc(lasso(G, ref_light_curve))
% 
% % EGI_guess = EGI_normals .* EGI_areas;
% 
% G_ref = computeReflectionMatrix(sun_vectors, viewer_vectors, ref_normals);
% ref_EGI_LC = G_ref * ref_areas;
% 
% cvx_begin
%     variable a(size(G, 2))
%     minimize(norm(ref_light_curve - G * a, 2))
% 
%     subject to
%         0 <= a;
% cvx_end
% % 
% nonzero_faces = abs(a) > 1e-2;
% EGI_normals_pre_resample = normal_vectors(nonzero_faces, :);
% EGI_areas_pre_resample = a(nonzero_faces);
% EGI_normals = EGI_normals_pre_resample;
% EGI_areas = EGI_areas_pre_resample;
% 
% for i = 1:length(EGI_normals)
%     EGI_dist = abs(repmat(EGI_normals(i, :), length(EGI_normals), 1) - EGI_normals);
%     EGI_dist(EGI_dist == 0) = NaN;
%     [closest_val, closest_ind(i)] = min(vecnorm(EGI_dist, 2, 2))
% end

%%%%% RESAMPLING NORMALS
% for i = 1
%     theta = pi/(40*i);
%     num = 100;
%     resampled_normals = [];
%     for j = 1:length(EGI_normals)
%         resampled_normals = [resampled_normals; sampleWithinCone(EGI_normals(j, :), theta, num)];
%     end
%     
%     G = computeReflectionMatrix(sun_vectors, viewer_vectors, resampled_normals);
%     
%     cvx_begin
%         variable a(size(G, 2))
%         minimize(norm(ref_light_curve - G * a, 2))
%     
%         subject to
%             0 <= a;
%     cvx_end
%     % 
%     nonzero_faces = a / max(a) > 0.2; %throw out smallest 20% of faces each time
%     EGI_normals = resampled_normals(nonzero_faces, :);
%     EGI_areas = a(nonzero_faces);
% end

%%%%% OPTIMIZING NORMAL POSITIONS AND WEIGHTINGS
% num_normals = 100;
% a0 = zeros(num_normals, 1);
% n0 = getNormalVectors(num_normals, "spiral");
% x0 = EGI_normals;
% 
% options = optimoptions('fmincon','Display','iter','Algorithm','sqp', ...
%     'maxfunctionevaluations', 10e3);
% estim = fmincon(@(x) LC_obj_shift(ref_EGI_LC, sun_vectors, viewer_vectors, EGI_areas, x), x0, ...
%     [],[],[],[],[],[], @LC_const, options); 


% normal_counts = round(linspace(10, 100, 1));
% face_counts = zeros(length(normal_counts), 1);
% fit_quality = zeros(length(normal_counts), 1);

% f = figure;
% [xx, yy, zz] = sphere;
% surf(xx, yy, zz, 'facealpha', '0', 'linestyle', ':')
% hold on
% RenderPatch(reference_obj.v / 3, reference_obj.f.v)
% 
% for i = 1:length(normal_counts)
% normal_vectors = [getNormalVectors(normal_counts(i), "spiral")];
% G = computeReflectionMatrix(sun_vectors, viewer_vectors, normal_vectors);
% 
% cvx_begin
%     variable a(size(G, 2))
%     minimize(norm(ref_light_curve - G * a, 2))
% 
%     subject to
%         0 <= a;
% cvx_end
% 
% nonzero_faces = abs(a) > 1e-2;
% EGI_normals = normal_vectors(nonzero_faces, :);
% EGI_areas = a(nonzero_faces);
% face_counts(i) = sum(nonzero_faces)
% fit_quality(i) = norm(ref_light_curve - G * a, 2);
% 
% if i == 1
%     egi_ax = scatter3(EGI_normals(:, 1), EGI_normals(:, 2), EGI_normals(:, 3), EGI_areas / max(EGI_areas)*200, 'filled');
%     axis equal
% else
%     egi_ax.XData = EGI_normals(:, 1);
%     egi_ax.YData = EGI_normals(:, 2);
%     egi_ax.ZData = EGI_normals(:, 3);
%     egi_ax.SizeData = EGI_areas / max(EGI_areas)*200;
% end
% texit(sprintf("EGI for $$%.0f$$ normals", normal_counts(i)), "", "")
% drawnow
% saveFrame(f, writerObj)
% end
% close(writerObj)

% scatter3(resampled_normals(:, 1), resampled_normals(:, 2), resampled_normals(:, 3), 'r.')
% scatter3(estim(:, 1), estim(:, 2), estim(:, 3), EGI_areas / max(EGI_areas)*200, 'filled')

% scatter3(normal_vectors(:, 1), normal_vectors(:, 2), normal_vectors(:, 3), estim / max(estim)*200, 'filled')
% scatter3(EGI_normals(:, 1), EGI_normals(:, 2), EGI_normals(:, 3), ...
%     EGI_areas / max(EGI_areas)*200, 'filled')
% scatter3(ref_normals(:, 1), ref_normals(:, 2), ref_normals(:, 3), ref_areas / max(ref_areas)*200, 'filled')
% scatter3(recon_EGI_ns(:, 1), recon_EGI_ns(:, 2), recon_EGI_ns(:, 3), recon_EGI_as / max(recon_EGI_as)*200, 'filled')
% RenderPatch(reference_obj.v / 3, reference_obj.f.v)
% axis equal
% texit("Resampled Normals", "", "")
% copyapp(105)

% figure
% plot(ref_light_curve, 'k', 'linewidth', 3)
% hold on
% plot(ref_EGI_LC, 'c--', 'linewidth', 1.25)
% texit("OpenGL LC vs EGI LC", "Time index", "Light curve function", ["OpenGL LC", "EGI LC"])

% 
% figure
% scatter(normal_counts, face_counts, "r*")
% texit("Face Counts for Normal Counts $$[10, 1000]$$", "Normal Count", "Least Squares EGI Face Count")
% copyapp(105)

%%%%% LASSO TRADE STUDY
lambdas = linspace(0, 1, 30);
a_sol_lambda = zeros(size(G, 2), length(lambdas));
fit_quality = zeros(length(lambdas));

% B = lasso(G, ref_EGI_LC);
% imagesc(B)

for i = 1:length(lambdas)
    cvx_begin quiet
        variable a(size(G, 2))
        minimize(norm(ref_light_curve - G * a, 2) + lambdas(i) * norm(a, 1))
    
        subject to
            0 <= a;
    cvx_end
    
    nonzero_faces = abs(a) > 1e-2;
    EGI_normals = normal_vectors(nonzero_faces, :);
    EGI_areas = a(nonzero_faces);
    
    fprintf("Faces used for lambda = %.2f: %d, fit: %.2f\n", lambdas(i), sum(nonzero_faces), ...
        norm(ref_light_curve - G * a, 2))
    a_sol_lambda(:, i) = a;
    fit_quality(i) = norm(ref_light_curve - G * a, 2);
end

imagesc(a_sol_lambda)
texit("LASSO Path For $$\lambda = [0, 1]$$", "Run Index", "Normal Index")

figure
plot(lambdas, fit_quality)
texit("LASSO Path Fit Quality For $$\lambda = [0, 1]$$", "$$\lambda$$", "$$\ell_2$$ residual error")

% reconstructed_obj = reconstructObj(EGI_guess);
% reconstructed_obj = solveFaceAreas(reconstructed_obj);
% reconstructed_obj = solveForUniqueNormalsAndAreas(reconstructed_obj);
% reconstructed_obj = solveEGI(reconstructed_obj);
% recon_EGI_as = vecnorm(reconstructed_obj.EGI, 2, 2);
% recon_EGI_ns = reconstructed_obj.EGI ./ recon_EGI_as;

function [c, ceq] = LC_const(x)
    c = [];
    ceq = vecnorm(x, 2, 2) - 1;
end

function loss = LC_obj_scale(ref_LC, G, a)
    lambda = 0.01;
    loss = norm(ref_LC - G * a, 2) + lambda * sum(abs(a) > 1e-5);
end

function loss = LC_obj_shift(ref_LC, sv, vv, a, x)
    G = computeReflectionMatrix(sv, vv, x);
    lambda = 0.01;
    loss = norm(ref_LC - G * a, 2) + lambda * sum(abs(a) > 1e-5);
end

function saveFrame(f, writerObj)
    % Capture the plot as an image 
    frame = getframe(f); 
    % Write to the AVI File 
    writeVideo(writerObj, frame);
end
