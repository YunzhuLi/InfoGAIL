psi=-.75;
theta=-0.4;#*pi;
d_psi = 0.005;
for i=1:200
  f(i,1)=psi;
  if (psi>0)
    f(i,2)=1;
    f(i,3)=1;
    f(i,4)=1;
    f(i,5)=1;
    f(i,6)=1;
    f(i,7)=1;
  elseif (abs(psi)>abs(2*theta))
    f(i,2)=0;
    f(i,3)=0;
    f(i,4)=0;
    f(i,5)=0;
    f(i,6)=0;
    f(i,7)=0;
  else
    f(i,2)=(1.0*(1-sin(psi)/sin(2*theta)))^2;
    f(i,3)=(0.5*(1+tanh((theta-psi)/(abs(1-(psi/theta))-1))))^2;
    f(i,4)=(0.5*(1+sin((theta-psi)*pi/(2*theta))))^2;
    f(i,5)=(1.0*(1-sin(psi)/sin(2*theta)));
    f(i,6)=(0.5*(1+tanh((theta-psi)/(abs(1-(psi/theta))-1))));
    f(i,7)=(0.5*(1+sin((theta-psi)*pi/(2*theta))));
  endif
  psi=psi+d_psi;

endfor

clearplot
hold on
#plot (f(:,1),f(:,2),'1')
#plot (f(:,1),f(:,3),'2')
#plot (f(:,1),f(:,4),'3')
plot (f(:,1),f(:,5),'1@')
plot (f(:,1),f(:,6),'2@')
plot (f(:,1),f(:,7),'3@')


	      
