CleanSlate

theta = [1/10, 1/20, 1/40]*pi;

num = 1e4

cone_axis = randUnitVectors(1)

cone_vec_rot1 = sampleWithinCone(cone_axis, theta(1), num)
cone_vec_rot2 = sampleWithinCone(cone_axis, theta(2), num)
cone_vec_rot3 = sampleWithinCone(cone_axis, theta(3), num)

scatter3(cone_vec_rot1(:, 1), cone_vec_rot1(:, 2), cone_vec_rot1(:, 3), '.')
hold on
scatter3(cone_vec_rot2(:, 1), cone_vec_rot2(:, 2), cone_vec_rot2(:, 3), '.')
scatter3(cone_vec_rot3(:, 1), cone_vec_rot3(:, 2), cone_vec_rot3(:, 3), '.')
[xx, yy, zz] = sphere;
surf(xx, yy, zz, 'facealpha', '0', 'linestyle', '-')
scatter3(cone_axis(1)*1.01, cone_axis(2)*1.01, cone_axis(3)*1.01, 100, '.')
texit("Uniform Cone Sampling", "", "")
