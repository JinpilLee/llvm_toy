double x[100];
double y[100];
double z[100];

double tx[100];
double ty[100];
double tz[100];

void foo() {
  for (int i = 0; i < 100; i++) {
    double tempx = x[i];
    double tempy = y[i];
    double tempz = z[i];

    double avg = tempx + tempy + tempz;

    double rx = tx[i];
    double ry = ty[i];
    double rz = tz[i];

    tx[i] = rx + avg;
    ty[i] = ry + avg;
    tz[i] = rz + avg;
  }
}
