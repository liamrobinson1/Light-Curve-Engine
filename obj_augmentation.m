CleanSlate

obj_name = "template_ico_sphere";
obj_file = "models/" + obj_name + ".obj";
mtl_file = false;
obj = readObj(obj_file, mtl_file);

% let's say we want to move vertex 1 by [1 0 0]
% we have to update its position and the face normals contained in its link

vertex_to_move = 1; %move the first vertex in the v list
vertex_aug = [0 -1 1]; %move the vertex by this much
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

writeObj(obj, "models/cube_tri_augmented.obj")

function writeObj(obj, fname)
    fid = fopen(fname, 'w');

    header = "# MATLAB writeObj() OBJ File: ''\n" + ...
             "o Cube\n";

    fprintf(fid, header);

    data = "";

    for i = 1:length(obj.v)
        data = data + sprintf("v %.6f %.6f %.6f\n", obj.v(i, :));
    end

    for i = 1:length(obj.vt)
        data = data + sprintf("vt %.6f %.6f\n", obj.vt(i, :));
    end

    for i = 1:length(obj.fn)
        data = data + sprintf("vn %.6f %.6f %.6f\n", obj.fn(i, :));
    end

    data = data + "s off\n";

    for i = 1:length(obj.f.v)
        face_mat = [obj.f.v(i, :); obj.f.vt(i, :); obj.f.fn(i, :)];
        data = data + sprintf("f %d/%d/%d %d/%d/%d %d/%d/%d\n", reshape(face_mat, 1, []));
    end

    fprintf(fid, data);

    fclose(fid);

    type models/cube_tri_augmented.obj
end
