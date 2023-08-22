
#ifndef HAVE_VKFFT_H
#define HAVE_VKFFT_H 1
void free_vkfft();
int64_t init_vkfft(std::size_t batches, std::size_t nfft,
                   std::size_t sample_id);
void vkfft_specgram_offload(arma::cx_fmat &Pw_in, arma::cx_fmat &Pw);
#endif
