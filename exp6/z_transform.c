void z_transform(double num[1], double den[3])
{
    /* Z 变换传递函数 H(z) = Y(z)/X(z) = 1 / (1 - 0.18 z^{-1} + 0.13 z^{-2}) */
    num[0] = 1.0;

    den[0] = 1.0;
    den[1] = -0.18;
    den[2] = 0.13;
}
