CleanSlate


[x,y,z] = meshgrid([-3:0.25:3]); 
V = x.*exp(-x.^2 -y.^2 -z.^2);
isosurface(x,y,z,V,1e-4);
