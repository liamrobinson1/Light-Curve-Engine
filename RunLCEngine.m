CleanSlate
rng default

data_points = 1000;

sun_vectors = 2*randUnitVectors(data_points);
viewer_vectors = 2*randUnitVectors(data_points);
model_file = "bs_smile.obj";
command_file = "light_curve.lcc";
results_file = "light_curve.lcr";
computation_method = "GPU";
dimensions = 15*60; %dimensions should be a multiple of 60
frame_rate = 1;

t = linspace(0, 2 * pi, data_points)';
sun_vectors = [0*t + 0.5, 0*t + 2, 1 + 0*t];
viewer_vectors = [sin(t) + 0*t, 0 + 0*t, cos(t) + 0*t];

viewer_vectors = viewer_vectors ./ vecnorm(viewer_vectors, 2, 2) * 2;
sun_vectors = sun_vectors ./ vecnorm(sun_vectors, 2, 2) * 2;
figure
hold on

index = 0;
for instances = [1]
    writeLCRFile(command_file, results_file, model_file, instances, dimensions, data_points, computation_method, ...
    sun_vectors, viewer_vectors, frame_rate)
    
    tic;
    [~, ~] = system("./LightCurveEngine");
    toc;

    light_curve_data = importdata(results_file);

    plot(1:data_points, light_curve_data, 'linewidth', 2);
    drawnow;
end

texit("Light Curve", "Data point index", "Light curve function $$L(\vec{o}, \vec{L})$$")

function unit_vectors = randUnitVectors(data_points)
    unit_vectors = reshape((rand(3, 1, data_points) - 0.5) * 2, data_points, 3);
    unit_vectors = unit_vectors ./ vecnorm(unit_vectors, 2, 2);
end

function writeLCRFile(command_file, results_file, model_file, instances, dimensions, ...
    data_points, computation_method, sun_vectors, viewer_vectors, frame_rate)
    f = fopen(command_file,'w');
    
    header = "Light Curve Command File\n" + ...
             sprintf("\nBegin header\n") + ...
             sprintf("%-20s %-20s\n", "Model File", model_file) + ...
             sprintf("%-20s %-20i\n", "Instances", instances) + ...
             sprintf("%-20s %-20i\n", "Square Dimensions", dimensions) + ...
             sprintf("%-20s %-20s\n", "Format", "SunXYZViewerXYZ") + ...
             sprintf("%-20s %-20s\n", "Reference Frame", "ObjectBody") + ...
             sprintf("%-20s %-20d\n", "Data Points", data_points) + ...
             sprintf("%-20s %-20s\n", "Computation Method", computation_method) + ...
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
