function obj = augmentModel(obj_file_path, vertices_to_move, vertex_augs)
    mtl_file = false;
    obj = readObj(obj_file_path, mtl_file);

    for i = 1:length(vertices_to_move)
        vertex_to_move = vertices_to_move(i);
        vertex_aug = vertex_augs(i, :);

        faces_with_vertex = logical(sum(obj.f.v == vertex_to_move, 2));
        
        obj.v(vertex_to_move, :) = obj.v(vertex_to_move, :) + vertex_aug; %actually move vertex
        
        effected_face_vertices = obj.f.v(faces_with_vertex, :);
        effected_face_ids = find(faces_with_vertex);
        
        for face = 1:length(effected_face_vertices)
            face_index = effected_face_ids(face);
            face_vertices = obj.v(effected_face_vertices(face, :), :);
            old_face_vertex_normals = obj.fn(obj.f.fn(face_index, :), :);
        
            v1_2_v2 = face_vertices(2, :) - face_vertices(1, :);
            v1_2_v3 = face_vertices(3, :) - face_vertices(1, :);
        
            new_face_normal = cross(v1_2_v2, v1_2_v3) / norm(cross(v1_2_v2, v1_2_v3));
        
            obj.fn(obj.f.fn(face_index, :), :) = repmat(new_face_normal, 3, 1); %set the new normals
        end
    end
    
    writeObj(obj, "models/augmented.obj")
end
