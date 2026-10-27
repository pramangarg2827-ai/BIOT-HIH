BIOT — Breathe Analysis Redefined
Team Echo Echo | HackInHills

BIOT is a proposed IoT-enabled smart-mask system designed to explore non-invasive breath analysis through gas-sensor measurements and machine-learning-based pattern recognition. The project aims to investigate whether patterns in volatile organic compounds (VOCs) and other breath-related biomarkers can support future respiratory health screening research.

Project status: Experimental prototype. Cancer detection is a research objective, not a clinically validated capability.

**Table of Contents**
1.Overview
2.Problem Statement
3.Proposed Solution
4.System Architecture
5.Hardware Components
6.Software and Machine Learning
7.Dataset
8.Workflow
9.Key Features
10.Installation and Setup
11.Project Structure
12.Future Scope
13.Limitations and Safety


Overview
Cancer remains a major global health challenge, and access to timely screening can be limited by cost, infrastructure, and the need for specialized clinical investigations.

Research into breath-based biomarkers has explored how volatile organic compounds and other chemical characteristics of exhaled breath may be associated with certain diseases.

BIOT proposes a wearable breath-monitoring platform that combines gas sensing, embedded electronics, and machine learning to investigate these patterns. Its intended direction is to provide accessible respiratory monitoring and support further research into breath-based screening technologies.

The project brings together three core areas:

**Internet of Things (IoT)**: Sensor acquisition and embedded data processing.

**Machine Learning (ML)**: Analysis of sensor measurements and experimental classification.

**Healthcare Technology**: A wearable concept for non-invasive respiratory monitoring research.

**Problem Statement**
Conventional cancer screening may require specialized equipment, clinical facilities, laboratory testing, and professional interpretation. These requirements can create accessibility challenges, particularly in underserved and remote communities.

Breath analysis is an active research area because exhaled breath contains chemical compounds that may reflect physiological processes. However, developing a reliable breath-based screening system requires appropriate sensors, calibrated measurements, representative clinical datasets, and rigorous validation.

BIOT explores a potential approach to this challenge by combining breath-related sensor measurements with machine learning in a portable, wearable system concept.

**Proposed Solution**
BIOT proposes a smart mask architecture that integrates gas-sensing hardware with an embedded processing unit and an AI-based analysis pipeline.

The intended system workflow is:

**Data acquisition**: Collect gas-sensor and environmental measurements.

**Preprocessing**: Organize readings and account for measurement noise and environmental conditions.

**Machine learning**: Analyze input features using a trained classification model.

**Output generation**: Present experimental predictions and sensor information through a software interface.

**Future connectivity**:  A dedicated, validated nitric oxide sensor (Alphasense NO-B4), historical monitoring, and clinician-supervised review.

**System Architecture**
The proposed system consists of the following layers.

**1. Sensing Layer**
Collects environmental and gas-related measurements using compatible sensors.

**2. Embedded Processing Layer**
Uses an ESP32 microcontroller for sensor acquisition, local preprocessing, and wireless communication where implemented.

**3. Machine Learning Layer**
Processes structured features through a trained classification model to generate an experimental prediction.

**4. Application Layer**
A planned dashboard or connected application can display sensor readings, model outputs, and measurement history.

**5. Data and Validation Layer**
Supports dataset management, performance evaluation, and future research using independently collected and appropriately labelled measurements.

**Hardware Components**
Component	Intended role
ESP32	Microcontroller for sensor acquisition and wireless connectivity
BME280	Temperature, humidity, and atmospheric pressure sensing
ENS160	Air-quality sensing, including TVOC and equivalent CO₂-related estimates
VOC sensor module	Gas-related measurements for experimental analysis
**Dedicated NO sensor**	Potential future measurement of nitric oxide, subject to hardware selection
USB power supply	Power during development and debugging

**Software Components**

**1.Machine Learning Model**: Trained on breath sample datasets to recognize cancer-specific VOC and NO patterns. Uses algorithms such as Support Vector Machines (SVM),Convolutional Neural Networks (CNN), or Random Forest classifiers for accurate predictions.

**2.Dashboard Application**: Displays real-time analysis, historical data, and alerts. Includes a user-friendly interface for monitoring breath health and seeking medical advice.

**3.Cloud Storage**: Optional feature for storing anonymized user data to improve model accuracy through continuous learning.

Dataset
The current development workflow uses a synthetic breath-sensor dataset named synthetic_breath_sensors_v2.csv.

The dataset includes the following fields:
age
smoking
tvoc_ppb
eco2_ppm
rh_pct
temp_c
label_cancer
session_id

The dataset supports experimentation with preprocessing, classification, and model evaluation.

Dataset limitations:

The records are synthetic rather than verified clinical measurements.
The target labels do not establish a medically confirmed cancer diagnosis.
Results from this dataset must not be interpreted as clinical diagnostic accuracy.

Future development requires appropriately documented breath measurements and independently verified clinical labels, with suitable permissions and privacy safeguards.

**Workflow**
The intended development workflow is:

Acquire or load sensor data.
Validate and preprocess the measurements.
Prepare features for machine learning.
Train a classification model.
Evaluate predictions on held-out data.
Save the trained model.
Integrate sensor acquisition and model inference into the application as the relevant components become available.

**Installation and Setup**

**Prerequisites**

1.Python 3.10 or a compatible version
2.pip
3.Git
4.The project's source files and dataset
5.Google Colab or a local Python environment

**1. Clone the repository**

git clone https://github.com/pramangarg2827-ai/BIOT-Smart-Breath-Analysis.git
cd BIOT-Smart-Breath-Analysis

**2. Create a virtual environment**

python -m venv venv
Activate it on Windows:

venv\Scripts\activate
On macOS or Linux:
source venv/bin/activate

**3. Install dependencies**

If a requirements.txt file is included:
pip install -r requirements.txt
Otherwise, install the packages used by the ML pipeline:

pip install pandas numpy scikit-learn matplotlib seaborn joblib
4. Prepare the dataset
Place the dataset in the location expected by your training script. Update the CSV path if necessary.

5. Train the model
Run the actual training script in your repository. For example, if the file is named train_model.py:

python train_model.py
The command above assumes that this script exists and is configured to load the dataset and train the model.

6. Run the application
If the project contains an application entry point such as app.py, run it using the command appropriate for your framework. For example:

python app.py
The application and hardware commands should be adjusted to match the implemented code.

After the setup is complete.

**Step-by-Step Instructions for Use**
1. Inhale deeply and exhale naturally into the mask. The sensors will collect the breath sample and measure the concentration of VOCs and
NO levels.Repeat this process 3-5 times for the AI to gather sufficient data for analysis.

2. The collected data is sent to the built-in AI model, which compares the VOC and NO levels with known cancer biomarker patterns.
Within a few seconds to minutes, the AI algorithm processes the results and determines if abnormal biomarker levels are present.

3. The results are displayed on the connected dashboard.The dashboard provides:
Green Signal: Normal biomarker levels; no cancer indication.
Yellow Signal: Slightly elevated levels; suggests monitoring and retesting after a few days.
Red Signal: High risk; recommends consulting a doctor for further screening.

4.Taking Action Based on Results
If Green (Normal): Continue regular monitoring as per recommended intervals.
If Yellow (Mild Risk): Retest after a few days and check if levels persist or increase.
If Red (High Risk): Seek immediate medical consultation for further clinical testing (CT scans, biopsies, etc.).



Suggested Project Structure
BIOT-Breath Analysis Redefined/
│
├── data/
│   └── README.md
│
├── models/
│   └── README.md
│
├── notebooks/
│   └── model_experiments.ipynb
│
├── src/
│   ├── train_model.py
│   ├── evaluate_model.py
│   └── predict.py
│
├── firmware/
│   └── README.md
│
├── app.py
├── requirements.txt
├── .gitignore
└── README.md
This is a suggested structure, not a representation of the current repository. Keep only the files and folders that exist or are planned for your implementation. Avoid committing large generated files or sensitive data unnecessarily.

**Future Scope**

Beyond cancer detection, the Smart Mask has the potential to be expanded for detecting multiple diseases and improving personalized healthcare.

**1. Multi-Disease Detection**: In addition to cancer, the mask could be adapted to detect respiratory diseases like asthma, tuberculosis, and COPD by analyzing different VOC and NO biomarker patterns.

**2. Personalized Health Monitoring**: The integration of AI-driven analytics can help track an individual's long-term health trends, providing early warnings for various medical conditions.
   
**3.Cloud-Based Data Sharing**: The breath analysis data collected by the mask can be securely shared with doctors and researchers, enabling remote healthcare services and AI-powered diagnostics.

**Limitations and Safety**
BIOT is an experimental prototype and research project. It is not a clinically validated medical device.
The current synthetic dataset cannot establish proper cancer-detection.
A machine learning prediction must not be treated as a diagnosis or used to make medical decisions.

Anyone concerned about cancer should consult a qualified healthcare professional and follow established screening recommendations.

Any future patient-data collection must follow appropriate consent, privacy, ethical, and regulatory requirements.
