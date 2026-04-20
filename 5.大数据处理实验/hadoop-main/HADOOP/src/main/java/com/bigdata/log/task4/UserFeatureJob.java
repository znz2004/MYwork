package com.bigdata.log.task4;

import java.io.IOException;
import java.util.Map;

import org.apache.hadoop.io.LongWritable;
import org.apache.hadoop.io.Text;
import org.apache.hadoop.mapreduce.Mapper;
import org.apache.hadoop.mapreduce.Reducer;

import com.bigdata.log.utils.LogParser;

/**
 * 任务4-1: 用户行为特征提取
 */
public class UserFeatureJob {

    /**
     * Mapper类: 提取用户特征并生成特征向量
     */
    public static class UserFeatureMapper extends Mapper<LongWritable, Text, Text, Text> {
        private Text outKey = new Text();
        private Text outValue = new Text();
        
        @Override
        protected void map(LongWritable key, Text value, Context context)
                throws IOException, InterruptedException {
            // 获取日志行内容
            String line = value.toString();
            
            // 过滤空行或格式不正确的行
            if (line == null || line.trim().isEmpty()) {
                return;
            }
            
            try {
                // 使用LogParser工具类解析日志行
                Map<String, String> logMap = LogParser.parseLogLine(line);
                
                // 检查解析是否成功
                if (logMap.containsKey("error")) {
                    return;
                }
                
                // 提取用户IP地址作为用户标识
                String userIP = logMap.get("remote_addr");
                if (userIP == null || userIP.isEmpty()) {
                    return;
                }
                
                // 提取用于特征工程的字段
                String hour = logMap.get("hour");
                String browserType = logMap.get("browser_type");
                String method = logMap.get("method");
                String bodyBytesSent = logMap.get("body_bytes_sent");
                
                // 构建特征向量
                // 特征1: 访问时间段 (0-5: 深夜, 6-11: 上午, 12-17: 下午, 18-23: 晚上)
                int timeFeature = -1;
                try {
                    int hourInt = Integer.parseInt(hour);
                    if (hourInt >= 0 && hourInt <= 5) {
                        timeFeature = 0; // 深夜
                    } else if (hourInt >= 6 && hourInt <= 11) {
                        timeFeature = 1; // 上午
                    } else if (hourInt >= 12 && hourInt <= 17) {
                        timeFeature = 2; // 下午
                    } else if (hourInt >= 18 && hourInt <= 23) {
                        timeFeature = 3; // 晚上
                    }
                } catch (NumberFormatException e) {
                    // 无效的小时数，使用默认值
                    timeFeature = -1;
                }
                
                // 特征2: 设备类型 (0: 移动设备, 1: PC端, 2: Web Bot, 3: 其他)
                int deviceFeature = 3; // 默认为其他
                if (browserType != null) {
                    if (browserType.equals("Mobile Browser")) {
                        deviceFeature = 0; // 移动设备
                    } else if (browserType.equals("Chrome") || 
                               browserType.equals("Firefox") || 
                               browserType.equals("Safari") || 
                               browserType.equals("Internet Explorer") || 
                               browserType.equals("Opera")) {
                        deviceFeature = 1; // PC端
                    } else if (browserType.equals("Web Bot")) {
                        deviceFeature = 2; // Web Bot
                    }
                }
                
                // 特征3: 请求方法 (0: GET, 1: POST, 2: HEAD, 3: 其他)
                int methodFeature = 3; // 默认为其他
                if (method != null) {
                    if (method.equals("GET")) {
                        methodFeature = 0;
                    } else if (method.equals("POST")) {
                        methodFeature = 1;
                    } else if (method.equals("HEAD")) {
                        methodFeature = 2;
                    }
                }
                
                // 特征4: 传输数据量 (0: 小, 1: 中, 2: 大)
                int bytesFeature = 0; // 默认为小
                try {
                    long bytes = Long.parseLong(bodyBytesSent);
                    if (bytes <= 1000) {
                        bytesFeature = 0; // 小
                    } else if (bytes <= 10000) {
                        bytesFeature = 1; // 中
                    } else {
                        bytesFeature = 2; // 大
                    }
                } catch (NumberFormatException e) {
                    // 无效的字节数，使用默认值
                    bytesFeature = 0;
                }
                
                // 检查是否所有特征都有效
                if (timeFeature != -1) {
                    // 设置输出键为用户IP
                    outKey.set(userIP);
                    
                    // 设置输出值为特征向量
                    String featureVector = timeFeature + "," + deviceFeature + "," + methodFeature + "," + bytesFeature;
                    outValue.set(featureVector);
                    
                    context.write(outKey, outValue);
                }
                
            } catch (Exception e) {
                // 异常处理
                context.getCounter("User Feature", "Parse exception").increment(1);
            }
        }
    }

    /**
     * Reducer类: 汇总每个用户的特征向量并计算平均值
     */
    public static class UserFeatureReducer extends Reducer<Text, Text, Text, Text> {
        private Text result = new Text();
        
        @Override
        protected void reduce(Text key, Iterable<Text> values, Context context)
                throws IOException, InterruptedException {
            int count = 0;
            double[] featureSum = new double[4]; // 4个特征的总和
            
            // 累加每个特征的值
            for (Text val : values) {
                String[] features = val.toString().split(",");
                if (features.length == 4) {
                    for (int i = 0; i < 4; i++) {
                        featureSum[i] += Double.parseDouble(features[i]);
                    }
                    count++;
                }
            }
            
            // 计算平均特征值
            if (count > 0) {
                StringBuilder featureVector = new StringBuilder();
                for (int i = 0; i < 4; i++) {
                    double avgFeature = featureSum[i] / count;
                    featureVector.append(String.format("%.2f", avgFeature));
                    if (i < 3) {
                        featureVector.append(",");
                    }
                }
                
                // 添加访问频次特征 (5: 访问频次, 0: 低频, 1: 中频, 2: 高频)
                int frequencyFeature;
                if (count <= 5) {
                    frequencyFeature = 0; // 低频
                } else if (count <= 20) {
                    frequencyFeature = 1; // 中频
                } else {
                    frequencyFeature = 2; // 高频
                }
                
                featureVector.append(",").append(frequencyFeature);
                
                result.set(featureVector.toString());
                context.write(key, result);
            }
        }
    }
} 