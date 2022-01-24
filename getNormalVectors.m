function normal_vectors = getNormalVectors(subdivisions)
    [ico_verts, ico_faces] = icosphere(subdivisions);
    normal_vectors = ico_verts;
end
