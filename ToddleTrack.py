import streamlit as st
import firebase_admin
from firebase_admin import credentials, db
from streamlit_folium import st_folium
import folium
from math import radians, sin, cos, sqrt, atan2
from streamlit_autorefresh import st_autorefresh

# ------------------- Firebase Setup -------------------
if not firebase_admin._apps:
    cred = credentials.Certificate(st.secrets["FIREBASE"])
    firebase_admin.initialize_app(cred, {
        'databaseURL': 'https://toddletrack-fd848-default-rtdb.asia-southeast1.firebasedatabase.app/'
    })

# ------------------- Page Config -------------------
st.set_page_config(page_title="Toddle Track - Parent Dashboard", layout="wide")

# ------------------- Guardian Info -------------------
st.sidebar.title("👨‍👩‍👧 Guardian Info")
guardian_name = st.sidebar.text_input("Guardian Name", "John Doe")
guardian_phone = st.sidebar.text_input("Phone", "+91-9999999999")
guardian_email = st.sidebar.text_input("Email", "parent@example.com")
guardian_address = st.sidebar.text_area("Address", "Amrita Vishwa Vidyapeetham, Coimbatore")

if st.sidebar.button("Update Info"):
    st.sidebar.success("Guardian Info Updated ✅")

# ------------------- Refresh Rate -------------------
refresh_rate = st.sidebar.slider("Refresh rate (seconds)", 2, 20, 5)
st_autorefresh(interval=refresh_rate * 1000, key="map_refresh")

# ------------------- Geofence -------------------
safe_radius = st.sidebar.slider("Geofence radius (meters)", 10, 2000, 100)

# ------------------- Fetch Child Location -------------------
def fetch_child_location():
    try:
        ref = db.reference("sensor")  # Node where Arduino sends data
        data = ref.get()
        if data and "latitude" in data and "longitude" in data:
            return float(data["latitude"]), float(data["longitude"])
        return None, None
    except Exception as e:
        st.error(f"Error fetching data: {e}")
        return None, None

# ------------------- Haversine Formula -------------------
def haversine(lat1, lon1, lat2, lon2):
    R = 6371000  # meters
    dlat = radians(lat2 - lat1)
    dlon = radians(lon2 - lon1)
    a = sin(dlat/2)**2 + cos(radians(lat1)) * cos(radians(lat2)) * sin(dlon/2)**2
    c = 2 * atan2(sqrt(a), sqrt(1-a))
    return R * c

# ------------------- Main Dashboard -------------------
lat, lon = fetch_child_location()
st.subheader("📍 Child Location")

if lat is not None and lon is not None:
    # Initialize safe zone if not already
    if "safe_lat" not in st.session_state:
        st.session_state.safe_lat, st.session_state.safe_lon = lat, lon
    if "trail" not in st.session_state:
        st.session_state.trail = []

    # Append current location to trail
    st.session_state.trail.append([lat, lon])

    # Map setup
    m = folium.Map(location=[lat, lon], zoom_start=17)

    # Child marker
    folium.CircleMarker(
        location=[lat, lon],
        radius=6,
        color="red",
        fill=True,
        fill_color="red",
    ).add_to(m)

    # Movement trail
    folium.PolyLine(st.session_state.trail, color="green").add_to(m)

    # Safe zone circle
    folium.Circle(
        location=[st.session_state.safe_lat, st.session_state.safe_lon],
        radius=safe_radius,
        color="blue",
        fill=True,
        fill_opacity=0.2
    ).add_to(m)

    # Update safe zone center on map click
    map_click = st_folium(m, height=500, width=900)
    if map_click and map_click["last_clicked"] is not None:
        st.session_state.safe_lat = map_click["last_clicked"]["lat"]
        st.session_state.safe_lon = map_click["last_clicked"]["lng"]

    # Display current location
    st.success(f"Child’s Current Location: 📍 ({lat}, {lon})")

    # Geofence check
    distance = haversine(lat, lon, st.session_state.safe_lat, st.session_state.safe_lon)
    if distance > safe_radius:
        st.error(f"⚠ ALERT: Child is outside the safe zone! (Distance: {int(distance)} m)")
    else:
        st.info(f"✅ Child is inside the safe zone (Distance: {int(distance)} m)")

else:
    st.warning("No location data available yet.")
