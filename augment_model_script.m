
%%%%% GENERATING GUESS LIGHT CURVE
guess_light_curve = runLightCurveEngine(command_file, results_file, opt_model_file, instances, dimensions, data_points, computation_method, ...
    sun_vectors, viewer_vectors, frame_rate);

plot(1:data_points, guess_light_curve, 'linewidth', 2);
drawnow

lc_error_new = sqrt(mean((ref_light_curve - guess_light_curve) .^ 2));

obj = readObj("models/" + opt_model_file, false);
vertices_to_move = [];
vertex_augs = zeros(0, 3);

for i = 1:1000
    %%%%% AUGMENTING VERTICES
    vertices_to_move(end+1) = ceil(rand() * length(obj.v));
    vertex_augs(end+1, :) = randVertexAugment(1) / 2;
    
    aug_model = augmentModel("models/" + opt_model_file, vertices_to_move, vertex_augs);
    
    augmented_model_file = "augmented.obj";
    
    guess_light_curve = runLightCurveEngine(command_file, results_file, augmented_model_file, instances, dimensions, data_points, computation_method, ...
        sun_vectors, viewer_vectors, frame_rate);
    
    lc_error_old = lc_error_new;
    lc_error_new = sqrt(mean((ref_light_curve - guess_light_curve) .^ 2));
    
    if lc_error_old < lc_error_new && rand > 0.0 %then we've gone backwards
        vertices_to_move(end) = [];
        vertex_augs(end, :) = [];
        lc_error_new = lc_error_old; %we should use the old error
    else %then we've gone forward
        fprintf("Best RMSE: %.4f\n", lc_error_new);
        writeObj(aug_model, "models/best_guess.obj")

        plot(1:data_points, guess_light_curve, 'linewidth', 2);
        drawnow
    end
end
