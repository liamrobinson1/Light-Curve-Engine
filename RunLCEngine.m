CleanSlate

% [~, ~] = system("./LightCurveEngine");

t = linspace(0, 2*pi)';
sun_vector = [sin(t), 0*t, cos(t)];
viewer_vector = [cos(t/2), 0*t, sin(t/2)];
instances = 12;

f = fopen('light_curve.lcc','w');

header = "Light Curve Command File\n" + ...
         sprintf("\nBegin header\n") + ...
         sprintf("%-20s %-20s\n", "Model File", "bob_tri.obj") + ...
         sprintf("%-20s %-20i\n", "Instances", instances) + ...
         sprintf("%-20s %-20s\n", "Format", "SunXYZViewerXYZ") + ...
         sprintf("%-20s %-20s\n", "Reference Frame", "ObjectBody") + ...
         sprintf("%-20s %-20d\n", "Data Points", length(sun_vector)) + ...
         sprintf("End header\n\n");

fprintf(f, header);

data = "Begin data\n";
for i = 1:length(sun_vector)
    data = data + sprintf("%-10f %-10f %-10f", sun_vector(i, :)) + ...
        sprintf("%-10f %-10f %-10f\n", viewer_vector(i, :));
end
data = data + "End data";

fprintf(f, data);

type 'light_curve.lcc'
fclose(f);
