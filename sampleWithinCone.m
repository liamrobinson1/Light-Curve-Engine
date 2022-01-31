function cone_vec_rot = sampleWithinCone(cone_axis, theta, num) %theta = cone half-angle
    r1 = rand(num, 1);
    r2 = rand(num, 1);
    z = (1-cos(theta)).*r1 + cos(theta);
    phi = (2*pi).*r2 + 0;
    
    cone_vec = [sqrt(1 - z .^ 2) .* cos(phi), sqrt(1 - z .^ 2) .* sin(phi), z];
    cone_vec_rot = zeros(size(cone_vec));
    rot_vector = -cross(cone_axis, [0; 0; 1]);
    
    rot_angle = acos(dot(cone_axis', [0; 0; 1]));
    
    rotm = axang2rotm([rot_vector rot_angle]);
    
    for i = 1:length(cone_vec)
        cone_vec_rot(i, :) = rotm * cone_vec(i, :)';
    end
end
