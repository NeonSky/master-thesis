#include "WaveSpectrums.h"
#include "Globals/StatelessHelpers.h"


float PhillipsWaveSpectrum(FVector2D k_vec, const FPhillipsSpectrumSettings& settings) {
  if (k_vec.IsZero()) {
    return 0.0f;
  }

  float phillips_L = pow(settings.wind_speed, 2.0) / GRAVITY;

  float wave_number = k_vec.Size(); // vector magnitude
  // float wave_number = 1.0f;

  FVector2D k_hat = k_vec.GetSafeNormal();

  float k_hat_dot_omega_hat = FVector2D::DotProduct(k_hat, settings.wind_direction);
  // float k_hat_dot_omega_hat = 1.0f;

  float res = settings.amplitude;
  res *= exp(-1.0 / pow(wave_number * phillips_L, 2.0));
  res /= pow(wave_number, 4.0);
  res *= pow(k_hat_dot_omega_hat, settings.wave_alignment);

  float damping   = 0.001f;
  float wave_length = wave_number * damping; // Used by Tessendorf in the first set of examples
  res *= exp(-pow(wave_number, 2.0) * pow(wave_length, 2.0));

  return res;
}

float JonswapFrequencySpectrum(float omega, const FJonswapSpectrumSettings& settings) {
  double U_10 = settings.wind_speed;
  double F = settings.fetch;
  double gamma = 3.3f;
  double g = GRAVITY;
  double alpha = 0.076f * pow(g * F / pow(U_10, 2.0f), -0.22f);
  double omega_p = 22.0f * pow(U_10 * F / pow(g, 2.0f), -0.33f);
  double sigma = omega <= omega_p ? 0.07f : 0.09f;
  double r = exp(-pow(omega - omega_p, 2.0) / (2.0 * sigma * sigma * omega_p * omega_p));

  return alpha * g * g * exp(-(5.0/4.0) * pow(omega_p / omega, 4.0)) * pow(gamma, r) / pow(omega, 5.0);
}

float JonswapWaveSpectrum(FVector2D k_vec, const FJonswapSpectrumSettings& settings) {
  if (k_vec.IsZero()) {
    return 0.0f;
  }

  float k = k_vec.Size();
  float g = GRAVITY;
  float omega = sqrt(k * g);

  return JonswapFrequencySpectrum(omega, settings) * sqrt(g) / (2 * k * sqrt(k));
}

float UniformDirectionalSpectrum() {
  return 1.0f / (2.0f * PI);
}

float DonelanBannerDirectionalSpectrum(float theta, float omega, const FDonelanBannerSettings& settings) {

  float U_10 = settings.wind_speed;
  float F = settings.fetch;
  float g = GRAVITY;

  float omega_p = 22.0f * pow(U_10 * F / pow(g, 2.0f), -0.33f);
  float omega_over_omega_p = omega / omega_p;

  float beta = 0.0f;
  if (omega_over_omega_p < 0.95f) {
    beta = 2.61f * pow(omega_over_omega_p, 1.3f);
  }
  else if (omega_over_omega_p < 1.6f) {
    beta = 2.28f * pow(omega_over_omega_p, -1.3f);
  }
  else {
    float e = -0.4 + 0.8393 * exp(-0.567 * log(pow(omega_over_omega_p, 2.0)));
    beta = pow(10, e);
  }

  float sech_term = 1.0f / cosh(beta * theta); // https://mathworld.wolfram.com/HyperbolicSecant.html

  return beta * pow(sech_term, 2.0) / (2 * tanh(beta * PI));
}