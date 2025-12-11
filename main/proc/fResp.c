#include "fResp.h"

static uint16_t smooth_win; 
static uint16_t center_win; 
static uint16_t n_update_once; 
static uint16_t slope_window; 
static uint16_t segment_duration; 
static uint16_t segment_samples; 
static uint16_t eval_interval; 
static uint16_t eval_step; 
static uint16_t current_sample_restriction;
bool has_threshold = false; // threshold flag

static circbuf_t *raw_buffer; 
static circbuf_t *smooth_buffer;
static circbuf_t *centered_buffer;


/**
 * Local functions
 */

 // Initialization of buffers and variables can be done here if needed
void fResp_init(void) {
    smooth_win = lrintf(0.5f * FS); // 500 ms smoothing - 0.5 sec
    center_win = lrintf(5.0f * FS); // 5 seconds for slow baseline drift
    n_update_once = lrintf(120.0f * FS); // Update time for threshold
    slope_window = lrintf(0.05f * FS);
    segment_duration = 10; // seconds
    segment_samples = lrintf(segment_duration * FS);
    eval_interval = 2; // seconds
    eval_step = lrintf(eval_interval * FS);
    current_sample_restriction = lrintf(5.0f * FS);

    raw_buffer = create_buffer(smooth_win);
    smooth_buffer = create_buffer(center_win);
    centered_buffer = create_buffer(segment_samples);
}

void processed_signal(float new_point, float *smp, float *cp)// circbuf_t *raw_buffer, circbuf_t *smooth_buffer, int smooth_win, int center_win){
{
    // % Add new data point to raw buffer
    // raw_buffer(end+1) = new_point;
    // % Keep buffer size within smooth_win
    // if length(raw_buffer) > smooth_win
    //     raw_buffer(1) = []; % remove oldest sample
    // end
    buffer_push(raw_buffer, new_point);

    // --- Smoothing (short window) ---
    // if length(raw_buffer) < smooth_win
    //     smoothed_point = mean(raw_buffer);
    // else
    //     smoothed_point = mean(raw_buffer(end-smooth_win+1:end));
    // end
    float smoothed_point = 0.0f;
    smoothed_point = mean(raw_buffer->buffer, raw_buffer->count);

    // Output smoothed value
    *smp = smoothed_point;

    // % Add smoothed point to smooth buffer
    // smooth_buffer(end+1) = smoothed_point;
    //     % Keep buffer size within center_win
    // if length(smooth_buffer) > center_win
    //     smooth_buffer(1) = [];
    // end
    buffer_push(smooth_buffer, smoothed_point);

    //     % --- Centering (long window) ---
    // if length(smooth_buffer) < center_win
    //     baseline = mean(smooth_buffer);
    // else
    //     baseline = mean(smooth_buffer(end-center_win+1:end));
    // end
    float baseline = 0.0f;
    baseline = mean(smooth_buffer->buffer, smooth_buffer->count);
    
    //     % Output centered value
    // centered_point = smoothed_point - baseline;
    // end
    *cp = smoothed_point - baseline;

}


/**
 * Find zero crossings in a signal
 * function [clean_crossings, valid_crossings, breaths_per_second,crossing_start_idx, crossing_end_idx] 
 *      = calculateRespFreqZerosCross_Ver2_online(centered_buffer,sample_restriction,slope_window, fs, amplitude_threshold)
 */
// void zeros_crossings(circbuf_t *c_b)//, uint16_t size, uint16_t *crossings, uint16_t *num_crossings)
void zeros_crossings(void)//circbuf_t *c_b)
{   
    /*
     * % Step 1: Detect raw zero crossings
     * product = signal(1:end-1) .* signal(2:end);
     * raw_crossings = find(product <= 0);
     */

    /*for debugging*/
    circbuf_t *c_b = centered_buffer;
    c_b->size = 10;
    c_b->tail = 3;
    for (uint16_t i = 0; i < (c_b->size); i++) {
        c_b->buffer[i] = (float)i; // Example data, replace with actual signal data
        printf("Buffer[%d]: %f\n", i, c_b->buffer[i]);
    }
    /*------------*/
    
    uint16_t i;
    float product[(c_b->size)-1];
    for (i = 0; i < (c_b->size)-1; i++) {
        product[i] = c_b->buffer[(i + c_b->tail)%c_b->size] 
                   * c_b->buffer[(i + c_b->tail + 1)%c_b->size];
    }

    uint16_t raw_crossings[(c_b->size)%100];
    uint16_t num_crossings = 0;

    /*for debugging*/
    for (uint16_t i = 0; i < (c_b->size-1); i++) {
       printf("Product[%d]: %f\n", i, product[i]);
    }
    /*------------*/

    // *num_crossings = 0;
    // for (uint16_t i = 1; i < size; i++) {
    //     if ((data[i-1] < 0 && data[i] >= 0) || (data[i-1] > 0 && data[i] <= 0)) {
    //         crossings[*num_crossings] = i;
    //         (*num_crossings)++;
    //         if (*num_crossings >= size) break; // Prevent overflow
    //     }
    // }
}

void rfrec(float data_value)
{   
    float s_val = 0.0f; 
    float c_val = 0.0f;
    // data_value = data(n);
    // [s_val, c_val, raw_buffer, smooth_buffer] = processed_signal(data_value, raw_buffer, smooth_buffer, smooth_win, center_win);
    processed_signal(data_value, &s_val, &c_val);

    // centered_buffer = [centered_buffer, c_val];
      
    // if length(centered_buffer) > segment_samples
    //     centered_buffer(1) = []; % keep only last center_win values
    // end
    buffer_push(centered_buffer, c_val);

}

/**
 * for n = 1:length(data)

       
        


        % Store for plotting
        smoothed_signal(end+1) = s_val;
        centered_signal(end+1) = c_val;
    
        % Evaluate respiratory frequency every eval interval seconds after
        % segment duration
        if n > segment_samples && mod(n, eval_step) == 0
            segment = centered_buffer(end - segment_samples + 1:end);
    
            if has_threshold
                [clean_crossings, valid_crossings, breaths_per_second, crossing_start_idx, crossing_end_idx] = ...
                    calculateRespFreqZerosCross_Ver2_online(segment, current_sample_restriction, slope_window, fs, amplitude_threshold);
            else
                [clean_crossings, valid_crossings, breaths_per_second, crossing_start_idx, crossing_end_idx] = ...
                    calculateRespFreqZerosCross_Ver2_online(segment, current_sample_restriction, slope_window, fs);
            end
    
            % Calculate amplitude between valid crossings
            for i = 2:length(valid_crossings)
                idx1 = valid_crossings(i-1);
                idx2 = valid_crossings(i);
    
                if idx1 > 0 && idx2 <= length(centered_buffer)
                    seg = centered_buffer(idx1:idx2);
                    amp = max(seg) - min(seg);
                    amplitudes_all{end+1} = amp;
                end
            end

            if ~isnan(breaths_per_second) && breaths_per_second > 0
                base_frequencies(end+1) = breaths_per_second * 60;  % breaths/s → breaths/min
            
            elseif ~isempty(base_frequencies)
                base_frequencies(end+1) = base_frequencies(end); 

            end
    
            % Store results with global indices only for review and visualization purposes.
            all_valid_crossings{end+1} = valid_crossings + (n - segment_samples);
            all_clean_crossings{end+1} = clean_crossings + (n - segment_samples);

            all_crossing_start_idx(end+1) = crossing_start_idx  + (n - segment_samples);
            all_crossing_end_idx(end+1) = crossing_end_idx  + (n - segment_samples);
            
        end
    
        % One-time update of restriction and threshold
        if n == n_update_once && ~isempty(base_frequencies)
            mean_bpm = mean(base_frequencies);
            current_sample_restriction = round(((60 / mean_bpm) / 2) / 2 * fs);
            disp(['[n=' num2str(n/fs) 's] → One-time update: sample_restriction = ' num2str(current_sample_restriction)]);
    
            amplitudes_base = [amplitudes_all{1}, amplitudes_all{2}];
            amplitude_threshold = mean(amplitudes_base) * 0.2;
            has_threshold = true; 
        end
    end
 * 
 */



 /**
  *  
    smooth_win = round(0.5 * fs);     % 500 ms smoothing - 0.5 sec
    center_win = round(5 * fs);       % 5 seconds for slow baseline drift
    n_update_once = round(120 * fs);  % Update time for threshold

    slope_window = 0.05 * fs;
    segment_duration = 10;            % seconds 
    segment_samples = round(segment_duration * fs);
    eval_interval = 2;                % seconds
    eval_step = round(eval_interval * fs);
    
    % Initialization
    % Buffers for function
    raw_buffer = [];
    smooth_buffer = [];
    centered_buffer = []; 

    smoothed_signal = [];
    centered_signal = [];
    base_frequencies = [];
    all_crossing_start_idx = [];
    all_crossing_end_idx = [];

    current_sample_restriction = round(0.5 * fs);
    all_valid_crossings = {};
    all_clean_crossings = {};


    amplitudes_all = {};
    has_threshold = false; % threshold
    
    for n = 1:length(data)

        data_value = data(n);
        
        [s_val, c_val, raw_buffer, smooth_buffer] = processed_signal(data_value, raw_buffer, smooth_buffer, smooth_win, center_win);
  * 
  */