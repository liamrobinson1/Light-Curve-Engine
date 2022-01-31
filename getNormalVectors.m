function normal_vectors = getNormalVectors(num_faces, sphere_type)
    switch sphere_type
        case "ico"
            [verts, ~] = icosphere(num_faces);
        case "spiral"
            [verts,~] = SpiralSampleSphere(num_faces);
        case "rand"
            verts = RandSampleSphere(num_faces);
    end
    normal_vectors = verts;
end
