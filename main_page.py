import streamlit as st
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import streamlit.components.v1 as components
import json
import datetime

st.sidebar.button('main page')

########## Section 1. Filtering the data
st.title("Water Consumption data sorted by dorm name")

# Define a dictionary of dorms and their corresponding floors
dorm_floor = {
    "Allen": ["Floor 1", "Floor 2", "Floor 3"],
    "ISR": ["Floor 1", "Floor 2", "Floor 3", "Floor 4", "Floor 5", "Floor 6"]
    # Add floors for other dorms as needed
}

# Create a dropdown menu to select a dom
selected_dorm = st.selectbox("Select a dorm", list(dorm_floor.keys()))

# Create a second dropdown menu to select a floor based on the selected dorm
selected_floor = st.selectbox("Select a floor", dorm_floor[selected_dorm])

time_options = "Last week", "Last month", "Last year"
# Create a third dropdown menu to select a time frame
selected_time = st.selectbox("Select a time frame", time_options)

# Display the selected dorm, floor, and tine frame
st.write("You selected dorm:", selected_dorm)
st.write("You selected floor:", selected_floor)
st.write("You time frame:", selected_time)

today = datetime.datetime.now()

d5 = st.date_input("date range without default", [datetime.date(today.year, today.month, today.day), datetime.date(today.year, today.month, today.day)])
st.write(d5)

################################# Add a calendar that enables viewing data from a specific date 

########## Section 2. Data visualizations
     ######## graphs with non-random data for allan floor one and two 
# average_data = {"Average across dorms": [3, 4, 5, 3, 4, 5, 6, 4, 3, 5, 4, 6, 5, 3, 4, 6, 5, 4, 3, 6]} ##average data
# chart_data_allan_F1 = {"Water usage in the specified location": [2, 4, 3, 3, 4, 6, 6, 4, 4, 5, 6, 6, 5, 6, 4, 6, 5, 4, 5, 6] } ##allan floor 1 data
# chart_data_allan_F2 = {"Water usage in the specified location": [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0] } ##allan floor 2 data
# st.header("Water usage in " + selected_dorm + " on " + selected_floor) ##first line graph 
# if selected_dorm == "Allen" and selected_floor == "Floor 1" :
#     chart_data = chart_data_allan_F1.copy()
#     chart_data.update(average_data)
# elif selected_dorm == "Allen" and selected_floor == "Floor 2" :
#     chart_data = chart_data_allan_F2.copy()
#     chart_data.update(average_data)
# else:
#     chart_data = pd.DataFrame(np.random.randn(20, 2), columns=["Water usage in the specified location", "Average across dorms"])

st.header("Water usage in " + selected_dorm + " on " + selected_floor)
chart_data = pd.DataFrame(np.random.randn(20, 2), columns=["Water usage in the specified location", "Average across dorms"])
st.line_chart(chart_data)

st.header("Top 3 showers by water consumption per day")

# Create a bulleted list with random info for demo:
bulleted_list = """
- Allen 2nd floor, shower 4: x liters/day
- ISR 5th floor, shower 9: x liters/day
- ISR 4th floor, shower 1: x liters/day
- Average water consumption across all dorms: x liters/day
"""
st.write(bulleted_list)


########################################################### Delete later. Examples
st.title("Examples (delete later)")

# Title ------------
df = pd.DataFrame({'Name1': [1,2,3],
                   'Name2': [1,2,3]})

dict = {'Weight':['kg','tons','lbs'],
         'Speed':['m/s', 'km/hr']}

st.subheader("Example 1")
background_color = st.slider("Select a background color", 0, 100)
my_bar = st.progress(background_color)

# Line Chart
st.subheader("Example 2")
chart_data = pd.DataFrame(np.random.randn(20, 2), columns=["A", "B"])
st.line_chart(chart_data)

# Bar Chart
st.subheader("Example 3")
bar_chart_data = pd.DataFrame({"Month": ["Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"], "Liters": [20, 40, 70, 85, 15, 150, 120, 182, 2, 10, 15, 20]})
st.bar_chart(bar_chart_data.set_index("Month"))

# Generate data with days of the month and random values between 0 and 500
days_of_month = range(1, 31)  # Assuming a month with 30 days
random_values = np.random.randint(0, 501, 30)  # Generate 30 random values between 0 and 50
# Create a DataFrame from the data
chart_data = pd.DataFrame({'Day': days_of_month, 'Value': random_values})

# Set the title
# Create a Vega-Lite chart
st.subheader("Example 4")
color = st.color_picker("Choose a color", "#ff5733")
month = st.date_input("Choose a date")
st.vega_lite_chart(
    chart_data,
    {
        "mark": {"type": "circle", "tooltip": True},
        "encoding": {
            "x": {"field": "Day", "type": "ordinal", "title": "Day of the Month"},
            "y": {"field": "Value", "type": "quantitative", "title": "Liters[L]"},
            "size": {"field": "Value", "type": "quantitative"},
            "color": {"value": color},  # Use the selected color,
        },
    },
)
# Matplotlib Chart
st.subheader("Example 5")
fig, ax = plt.subplots()
ax.plot(["S", "M", "T", "W", "Th", "F", "Sat"], [140, 150, 230, 14, 95, 20, 11])
st.pyplot(fig)

# Header
st.header("File Upload")
# File Uploader
uploaded_file = st.file_uploader("Upload a file", type=["csv", "txt"])
if uploaded_file is not None:
    st.write("File Uploaded!")
    df = pd.read_csv(uploaded_file)
    st.write(df)
