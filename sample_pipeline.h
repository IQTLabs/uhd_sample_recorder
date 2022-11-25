void set_sample_buffer_capacity(size_t buffer_ptr, size_t buffer_size);
char *get_sample_buffer(size_t buffer_ptr, size_t *buffer_capacity);
void enqueue_samples(size_t &buffer_ptr);
void sample_pipeline_start(const std::string &file, const std::string &fft_file, size_t max_samples_, size_t zlevel, bool useVkFFT_, size_t nfft_, size_t nfft_overlap_, size_t nfft_div, size_t nfft_ds_, size_t rate, size_t batches, size_t sample_id);
size_t get_samp_size();
void sample_pipeline_stop(size_t overflows);
void set_sample_pipeline_types(const std::string &type, std::string &cpu_format);
