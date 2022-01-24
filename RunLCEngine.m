CleanSlate

data_points = 100;

reference_model_file = "warped_cube.obj";
command_file = "light_curve.lcc";
results_file = "light_curve.lcr";
dimensions = 15*60; %dimensions should be a multiple of 60
frame_rate = 500;
instances = 5;
reference_obj = readObj("models/" + reference_model_file, "false");

t = linspace(0, 2 * pi, data_points)';
% sun_vectors = [cos(2*t) + 0*t, -cos(4*t) + 0*t, 0*t + sin(2*t)];
% viewer_vectors = [sin(t) + 0*t, cos(t) + 0*t, sin(5*t) + 0 * t];

sun_vectors = [cos(2*t) + 0*t, -cos(4*t) + 0*t, 0*t + sin(2*t)];
viewer_vectors = [sin(t) + 0*t, cos(t) + 0*t, sin(5*t) + 0 * t];

% sun_vectors = randUnitVectors(data_points);
% viewer_vectors = randUnitVectors(data_points);

viewer_vectors = viewer_vectors ./ vecnorm(viewer_vectors, 2, 2) * 2;
sun_vectors = sun_vectors ./ vecnorm(sun_vectors, 2, 2) * 2;
normal_vectors = getNormalVectors(3);
G = computeReflectionMatrix(sun_vectors, viewer_vectors, normal_vectors);

% %%%%% GENERATING REFERENCE LIGHT CURVE
ref_light_curve = runLightCurveEngine(command_file, results_file, reference_model_file, instances, dimensions, data_points, ...
    sun_vectors, viewer_vectors, frame_rate);

cvx_begin
    variable a(size(G, 2))
    convex_light_curve = G * a;
    minimize(norm(ref_light_curve - convex_light_curve, 1))

    subject to
        0 <= a;
cvx_end

% cvx_begin
%     variable a(size(G, 2))
%     convex_light_curve = G * a;
%     light_curve_residual = ref_light_curve - convex_light_curve;
%     minimize(norm(a, 1))
% 
%     subject to
%         0 <= a;
%         norm(light_curve_residual) <= 1e-2
% cvx_end

nonzero_faces = abs(a) > 1e-7;

EGI_guess = normal_vectors(nonzero_faces, :) .* a(nonzero_faces)
reference_obj = solveFaceAreas(reference_obj);
reference_obj = solveForUniqueNormalsAndAreas(reference_obj);
reference_obj = solveEGI(reference_obj);

reconstructed_obj = reconstructObj(EGI_guess)

% figure
% scatter3(reconstructed_obj.EGI(:, 1), reconstructed_obj.EGI(:, 2), reconstructed_obj.EGI(:, 3))
% hold on
% scatter3(EGI_guess(:, 1) / 0.1545, EGI_guess(:, 2) / 0.1545, EGI_guess(:, 3) / 0.1545)


figure
reconstructed_patch = RenderPatch(reconstructed_obj.v, reconstructed_obj.f.v);
figure
reconstructed_patch = RenderPatch(reference_obj.v, reference_obj.f.v);

figure
hold on

legendarr = ["Reference LC", "Convex EGI LC"];

scatter(1:data_points, ref_light_curve, 100, '.');
scatter(1:data_points, convex_light_curve, 100, '.');

% load('/Users/liam/OneDrive - purdue.edu/Frueh Research/Concave Object Light Curve/Concave-Object-Reconstruction-from-Light-Curve/ray_lc')
% scatter(linspace(0, data_points, length(L_obj)), L_obj, 100, '.')
texit("Box-Wing Sat Light Curve - Liam Robinson", "Data point index", "Light Curve Function", ...
    legendarr, 'southwest')

function light_curve = runLightCurveEngine(command_file, results_file, model_file, instances, dimensions, data_points, ...
    sun_vectors, viewer_vectors, frame_rate)

    writeLCRFile(command_file, results_file, model_file, instances, dimensions, data_points, ...
    sun_vectors, viewer_vectors, frame_rate)
    
    tic;
    [~, ~] = system("./LightCurveEngine");
    toc;

    light_curve = importdata(results_file);
end

function vertex_aug = randVertexAugment(data_points)
    vertex_aug = reshape((rand(1, 3, data_points) - 0.5) * 2, data_points, 3);
end

function unit_vectors = randUnitVectors(data_points)
    unit_vectors = reshape((rand(3, 1, data_points) - 0.5) * 2, data_points, 3);
    unit_vectors = unit_vectors ./ vecnorm(unit_vectors, 2, 2);
end

function writeLCRFile(command_file, results_file, model_file, instances, dimensions, ...
    data_points, sun_vectors, viewer_vectors, frame_rate)
    f = fopen(command_file,'w');
    
    header = "Light Curve Command File\n" + ...
             sprintf("\nBegin header\n") + ...
             sprintf("%-20s %-20s\n", "Model File", model_file) + ...
             sprintf("%-20s %-20i\n", "Instances", instances) + ...
             sprintf("%-20s %-20i\n", "Square Dimensions", dimensions) + ...
             sprintf("%-20s %-20s\n", "Format", "SunXYZViewerXYZ") + ...
             sprintf("%-20s %-20s\n", "Reference Frame", "ObjectBody") + ...
             sprintf("%-20s %-20d\n", "Data Points", data_points) + ...
             sprintf("%-20s %-20s\n", "Expected .lcr Name", results_file) + ...
             sprintf("%-20s %-20d\n", "Target Framerate", frame_rate) + ...
             sprintf("End header\n\n");
    
    fprintf(f, header);

    model_augmentation = "Begin model augmentation\n";
    model_augmentation = model_augmentation + sprintf("1 %-10f %-10f %-10f", [-2, 2, 0]);
    model_augmentation = model_augmentation + "\nEnd model augmentation\n\n";

    fprintf(f, model_augmentation);

    data = "Begin data\n";
    for i = 1:data_points
        data = data + sprintf("%-10f %-10f %-10f", sun_vectors(i, :)) + ...
            sprintf("%-10f %-10f %-10f\n", viewer_vectors(i, :));
    end
    data = data + "End data";
    
    fprintf(f, data);
    fclose(f);
end
