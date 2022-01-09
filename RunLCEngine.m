CleanSlate

data_points = 100;

reference_model_file = "box_wing_sat.obj";
opt_model_file = "template_ico_sphere.obj";
command_file = "light_curve.lcc";
results_file = "light_curve.lcr";
dimensions = 15*60; %dimensions should be a multiple of 60
frame_rate = 1000;
instances = 4;

t = linspace(0, 2 * pi, data_points)';

sun_vectors = [sin(t) + 0*t, 0 + 0*t, cos(t) + 0*t];
viewer_vectors = [sin(t) + 0*t, 0 + 0*t, cos(t) + 0*t];

viewer_vectors = viewer_vectors ./ vecnorm(viewer_vectors, 2, 2) * 2;
sun_vectors = sun_vectors ./ vecnorm(sun_vectors, 2, 2) * 2;

figure
hold on

%%%%% GENERATING REFERENCE LIGHT CURVE
ref_light_curve = runLightCurveEngine(command_file, results_file, reference_model_file, instances, dimensions, data_points, ...
    sun_vectors, viewer_vectors, frame_rate);

plot(1:data_points, ref_light_curve, 'linewidth', 2);
drawnow

texit("Light Curve", "Data point index", "Light curve function $$L(\vec{o}, \vec{L})$$")


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
