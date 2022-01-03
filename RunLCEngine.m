CleanSlate

rng default

data_points = 1000;
model_file = "bob_tri.obj";
instances = 1;

t = linspace(0, 2*pi, data_points)';
sun_vector = [sin(t/10), 0*t, cos(t/10)]*2;
viewer_vector = sun_vector;

% sun_vector = reshape(rand(3, 1, data_points) * 3 - 2, data_points, 3);
% viewer_vector = reshape(rand(3, 1, data_points) * 3, data_points, 3);

f = fopen('light_curve.lcc','w');

header = "Light Curve Command File\n" + ...
         sprintf("\nBegin header\n") + ...
         sprintf("%-20s %-20s\n", "Model File", model_file) + ...
         sprintf("%-20s %-20i\n", "Instances", instances) + ...
         sprintf("%-20s %-20s\n", "Format", "SunXYZViewerXYZ") + ...
         sprintf("%-20s %-20s\n", "Reference Frame", "ObjectBody") + ...
         sprintf("%-20s %-20d\n", "Data Points", data_points) + ...
         sprintf("End header\n\n");

fprintf(f, header);

data = "Begin data\n";
for i = 1:length(sun_vector)
    data = data + sprintf("%-10f %-10f %-10f", sun_vector(i, :)) + ...
        sprintf("%-10f %-10f %-10f\n", viewer_vector(i, :));
end
data = data + "End data";

fprintf(f, data);

% type 'light_curve.lcc'
fclose(f);


[~, ~] = system("./LightCurveEngine");
