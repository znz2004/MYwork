package com.bigdata.log.task4;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Random;

import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.FSDataInputStream;
import org.apache.hadoop.fs.FileSystem;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.io.LongWritable;
import org.apache.hadoop.io.Text;
import org.apache.hadoop.mapreduce.Mapper;
import org.apache.hadoop.mapreduce.Reducer;

/**
 * 任务4-2: 用户分群
 * 使用K-means算法对用户进行聚类分析
 */
public class UserClusteringJob {

    // K-means聚类的簇数量
    private static final int K = 5;
    // K-means算法迭代次数
    private static final int MAX_ITERATIONS = 20;
    // 收敛阈值
    private static final double CONVERGENCE_THRESHOLD = 0.001;

    /**
     * Mapper类: 计算每个用户特征向量与各个聚类中心的距离，并分配到最近的聚类
     */
    public static class UserClusteringMapper extends Mapper<LongWritable, Text, Text, Text> {
        private Text outKey = new Text();
        private Text outValue = new Text();
        
        // 聚类中心
        private double[][] centroids;
        
        @Override
        protected void setup(Context context) throws IOException, InterruptedException {
            // 初始化聚类中心
            Configuration conf = context.getConfiguration();
            String centroidsPath = conf.get("centroids.path");
            
            if (centroidsPath != null && !centroidsPath.isEmpty()) {
                // 从HDFS读取聚类中心
                centroids = loadCentroidsFromHDFS(centroidsPath, conf);
            } else {
                // 初始化随机聚类中心
                centroids = initializeRandomCentroids();
            }
        }
        
        @Override
        protected void map(LongWritable key, Text value, Context context)
                throws IOException, InterruptedException {
            String line = value.toString();
            String[] parts = line.split("\\t");
            
            if (parts.length != 2) {
                return;
            }
            
            String userIP = parts[0];
            String featureStr = parts[1];
            
            try {
                // 解析特征向量
                double[] features = parseFeatureVector(featureStr);
                
                // 找到最近的聚类中心
                int nearestCentroidIndex = findNearestCentroid(features, centroids);
                
                // 输出格式: 聚类ID作为key, 用户IP和特征向量作为value
                outKey.set(String.valueOf(nearestCentroidIndex));
                outValue.set(userIP + "\t" + featureStr);
                
                context.write(outKey, outValue);
                
            } catch (Exception e) {
                // 异常处理
                context.getCounter("User Clustering", "Parse exception").increment(1);
            }
        }
        
        /**
         * 解析特征向量字符串为double数组
         */
        private double[] parseFeatureVector(String featureStr) {
            String[] parts = featureStr.split(",");
            double[] features = new double[parts.length];
            
            for (int i = 0; i < parts.length; i++) {
                features[i] = Double.parseDouble(parts[i]);
            }
            
            return features;
        }
        
        /**
         * 找到距离特征向量最近的聚类中心
         */
        private int findNearestCentroid(double[] features, double[][] centroids) {
            double minDistance = Double.MAX_VALUE;
            int nearestIndex = 0;
            
            for (int i = 0; i < centroids.length; i++) {
                double distance = calculateDistance(features, centroids[i]);
                if (distance < minDistance) {
                    minDistance = distance;
                    nearestIndex = i;
                }
            }
            
            return nearestIndex;
        }
        
        /**
         * 计算两个向量之间的欧几里得距离
         */
        private double calculateDistance(double[] v1, double[] v2) {
            double sum = 0.0;
            
            // 计算欧几里得距离的平方
            for (int i = 0; i < v1.length && i < v2.length; i++) {
                double diff = v1[i] - v2[i];
                sum += diff * diff;
            }
            
            return Math.sqrt(sum);
        }
        
        /**
         * 从HDFS加载聚类中心
         */
        private double[][] loadCentroidsFromHDFS(String path, Configuration conf) throws IOException {
            double[][] centroids = new double[K][];
            FileSystem fs = FileSystem.get(conf);
            Path centroidsPath = new Path(path);
            
            if (fs.exists(centroidsPath)) {
                FSDataInputStream inputStream = fs.open(centroidsPath);
                BufferedReader reader = new BufferedReader(new InputStreamReader(inputStream));
                
                String line;
                int i = 0;
                while ((line = reader.readLine()) != null && i < K) {
                    centroids[i] = parseFeatureVector(line);
                    i++;
                }
                
                reader.close();
            } else {
                // 如果文件不存在，使用随机聚类中心
                centroids = initializeRandomCentroids();
            }
            
            return centroids;
        }
        
        /**
         * 初始化随机聚类中心
         */
        private double[][] initializeRandomCentroids() {
            double[][] centroids = new double[K][5]; // 5维特征向量
            Random random = new Random();
            
            for (int i = 0; i < K; i++) {
                // 时间段特征 (0-3)
                centroids[i][0] = random.nextInt(4);
                
                // 设备类型特征 (0-3)
                centroids[i][1] = random.nextInt(4);
                
                // 请求方法特征 (0-3)
                centroids[i][2] = random.nextInt(4);
                
                // 传输数据量特征 (0-2)
                centroids[i][3] = random.nextInt(3);
                
                // 访问频次特征 (0-2)
                centroids[i][4] = random.nextInt(3);
            }
            
            return centroids;
        }
    }

    /**
     * Reducer类: 重新计算聚类中心
     */
    public static class UserClusteringReducer extends Reducer<Text, Text, Text, Text> {
        private Text result = new Text();
        
        @Override
        protected void reduce(Text key, Iterable<Text> values, Context context)
                throws IOException, InterruptedException {
            // 聚类ID
            String clusterId = key.toString();
            
            List<String> users = new ArrayList<>();
            List<double[]> featureVectors = new ArrayList<>();
            
            // 收集该聚类中的所有用户
            for (Text val : values) {
                String[] parts = val.toString().split("\\t");
                if (parts.length == 2) {
                    String userIP = parts[0];
                    String featureStr = parts[1];
                    
                    users.add(userIP);
                    
                    // 解析特征向量
                    double[] features = parseFeatureVector(featureStr);
                    featureVectors.add(features);
                }
            }
            
            // 计算新的聚类中心
            double[] newCentroid = calculateCentroid(featureVectors);
            
            // 为聚类中心确定标签
            String clusterLabel = generateClusterLabel(newCentroid);
            
            // 输出聚类信息
            StringBuilder sb = new StringBuilder();
            sb.append("Cluster Label: ").append(clusterLabel).append("\n");
            sb.append("Centroid: ").append(formatFeatureVector(newCentroid)).append("\n");
            sb.append("User Count: ").append(users.size()).append("\n");
            sb.append("Sample Users: ");
            
            // 输出样本用户（最多10个）
            int sampleSize = Math.min(10, users.size());
            for (int i = 0; i < sampleSize; i++) {
                sb.append(users.get(i));
                if (i < sampleSize - 1) {
                    sb.append(", ");
                }
            }
            
            result.set(sb.toString());
            context.write(key, result);
        }
        
        /**
         * 解析特征向量字符串为double数组
         */
        private double[] parseFeatureVector(String featureStr) {
            String[] parts = featureStr.split(",");
            double[] features = new double[parts.length];
            
            for (int i = 0; i < parts.length; i++) {
                features[i] = Double.parseDouble(parts[i]);
            }
            
            return features;
        }
        
        /**
         * 计算聚类中心
         */
        private double[] calculateCentroid(List<double[]> vectors) {
            if (vectors.isEmpty()) {
                return new double[5]; // 返回全0向量
            }
            
            int dimensions = vectors.get(0).length;
            double[] centroid = new double[dimensions];
            
            // 计算各维度的平均值
            for (double[] vector : vectors) {
                for (int i = 0; i < dimensions && i < vector.length; i++) {
                    centroid[i] += vector[i];
                }
            }
            
            for (int i = 0; i < dimensions; i++) {
                centroid[i] /= vectors.size();
            }
            
            return centroid;
        }
        
        /**
         * 格式化特征向量
         */
        private String formatFeatureVector(double[] vector) {
            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < vector.length; i++) {
                sb.append(String.format("%.2f", vector[i]));
                if (i < vector.length - 1) {
                    sb.append(",");
                }
            }
            return sb.toString();
        }
        
        /**
         * 生成聚类标签
         */
        private String generateClusterLabel(double[] centroid) {
            // 基于聚类中心的特征值确定用户群体类型
            StringBuilder label = new StringBuilder();
            
            // 特征1: 访问时间段
            if (centroid[0] < 1.0) {
                label.append("深夜");
            } else if (centroid[0] < 2.0) {
                label.append("上午");
            } else if (centroid[0] < 3.0) {
                label.append("下午");
            } else {
                label.append("晚上");
            }
            
            // 特征2: 设备类型
            if (centroid[1] < 0.5) {
                label.append("移动端");
            } else if (centroid[1] < 1.5) {
                label.append("PC端");
            } else if (centroid[1] < 2.5) {
                label.append("爬虫");
            } else {
                label.append("其他设备");
            }
            
            // 特征5: 访问频次
            if (centroid[4] < 0.5) {
                label.append("低频");
            } else if (centroid[4] < 1.5) {
                label.append("中频");
            } else {
                label.append("高频");
            }
            
            label.append("用户群");
            
            return label.toString();
        }
    }
} 