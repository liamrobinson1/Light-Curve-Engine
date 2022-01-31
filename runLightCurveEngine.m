function light_curve = runLightCurveEngine(command_file, results_file, model_file, instances, dimensions, data_points, ...
    sun_vectors, viewer_vectors, frame_rate)

    writeLCRFile(command_file, results_file, model_file, instances, dimensions, data_points, ...
    sun_vectors, viewer_vectors, frame_rate)
    
    tic;
    [~, ~] = system("./LightCurveEngine");
    toc;

    light_curve = importdata(results_file);
end
