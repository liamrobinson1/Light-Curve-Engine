CleanSlate

data_points = 1000;

reference_model_file = "box_wing_sat_tri.obj";
command_file = "light_curve.lcc";
results_file = "light_curve.lcr";
dimensions = 15*60; %dimensions should be a multiple of 60
frame_rate = 1;

t = linspace(0, 2 * pi, data_points)';

sun_vectors = [cos(t) + 0*t, -cos(t) + 0*t, 0*t + sin(t)];
viewer_vectors = [1 + 0*t, 1 + 0*t, 1 + 0 * t];

viewer_vectors = viewer_vectors ./ vecnorm(viewer_vectors, 2, 2) * 2;
sun_vectors = sun_vectors ./ vecnorm(sun_vectors, 2, 2) * 2;

viewer_vectors = repmat(viewer_vectors(75, :), length(viewer_vectors), 1);
sun_vectors = repmat(sun_vectors(75, :), length(sun_vectors), 1);

figure
hold on
legendarr = [];

i = 1;
for instances = [16]
    %%%%% GENERATING REFERENCE LIGHT CURVE
    ref_light_curve(:, i) = runLightCurveEngine(command_file, results_file, reference_model_file, instances, dimensions, data_points, ...
        sun_vectors, viewer_vectors, frame_rate);
    
    legendarr = [legendarr sprintf("%d instances", instances)];
%     scatter(1:data_points, ref_light_curve(:, i), 100, '.');
    i = i + 1;
end
scatter(1:length(sun_vectors), ref_light_curve(:, 1) - ref_light_curve(:, 2), '.')

texit("Box-Wing Sat LC Pre-Fix - Liam Robinson", "Data point index", "Light Curve Function", ...
    "1 - 16 instance residual", 'southwest')

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
